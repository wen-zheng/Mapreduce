// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/tools/quic/quic_spdy_client_base.h"

#include "net/quic/core/crypto/quic_random.h"
#include "net/quic/core/quic_server_id.h"
#include "net/quic/core/spdy_utils.h"
#include "net/quic/platform/api/quic_flags.h"
#include "net/quic/platform/api/quic_logging.h"
#include "net/quic/platform/api/quic_ptr_util.h"
#include "net/quic/platform/api/quic_text_utils.h"

using base::StringToInt;
using std::string;

#include <iostream>
using std::cerr;
using std::endl;

namespace net {

  void QuicSpdyClientBase::ClientQuicDataToResend::Resend() {
    client_->SendRequest(*headers_, body_, fin_);
    headers_ = nullptr;
  }

  QuicSpdyClientBase::QuicDataToResend::QuicDataToResend(
      std::unique_ptr<SpdyHeaderBlock> headers,
      QuicStringPiece body,
      bool fin)
    : headers_(std::move(headers)), body_(body), fin_(fin) {}

  QuicSpdyClientBase::QuicDataToResend::~QuicDataToResend() {}

  QuicSpdyClientBase::QuicSpdyClientBase(
      const QuicServerId& server_id,
      const QuicVersionVector& supported_versions,
      const QuicConfig& config,
      QuicConnectionHelperInterface* helper,
      QuicAlarmFactory* alarm_factory,
      std::unique_ptr<NetworkHelper> network_helper,
      std::unique_ptr<ProofVerifier> proof_verifier)
    : QuicClientBase(server_id,
        supported_versions,
        config,
        helper,
        alarm_factory,
        std::move(network_helper),
        std::move(proof_verifier)),
    store_response_(false),
    latest_response_code_(-1) {}

  QuicSpdyClientBase::~QuicSpdyClientBase() {
    // We own the push promise index. We need to explicitly kill
    // the session before the push promise index goes out of scope.
    ResetSession();
  }

  QuicSpdyClientSession* QuicSpdyClientBase::client_session() {
    return static_cast<QuicSpdyClientSession*>(QuicClientBase::session());
  }

  void QuicSpdyClientBase::InitializeSession() {
    client_session()->Initialize();
    client_session()->CryptoConnect();
  }

  void QuicSpdyClientBase::OnClose(QuicSpdyStream* stream) {
    DCHECK(stream != nullptr);
    QuicSpdyClientStream* client_stream =
      static_cast<QuicSpdyClientStream*>(stream);

    const SpdyHeaderBlock& response_headers = client_stream->response_headers();
    if (response_listener_ != nullptr) {
      response_listener_->OnCompleteResponse(stream->id(), response_headers,
          client_stream->data());
    }

    // Store response headers and body.
    if (store_response_) {
      auto status = response_headers.find(":status");
      if (status == response_headers.end() ||
          !QuicTextUtils::StringToInt(status->second, &latest_response_code_)) {
        QUIC_LOG(ERROR) << "Invalid response headers";
      }
      latest_response_headers_ = response_headers.DebugString();
      preliminary_response_headers_ =
        client_stream->preliminary_headers().DebugString();
      latest_response_header_block_ = response_headers.Clone();
      latest_response_body_ = client_stream->data();
      const auto &body = latest_response_body_;
      bool ok = (QuicTextUtils::Uint64ToString(body.size()) == response_headers.find("content-length")->second);

      unsigned seed = 48151623;
      for(char c : body) {
        ok = ok && (c == char(seed % 256));
        seed = seed * 47u;
      }
      // cerr << "Received packet. ok = " << ok << endl;
      assert(ok);
      latest_response_trailers_ =
        client_stream->received_trailers().DebugString();
    }
  }

  std::unique_ptr<QuicSession> QuicSpdyClientBase::CreateQuicClientSession(
      QuicConnection* connection) {
    return QuicMakeUnique<QuicSpdyClientSession>(*config(), connection,
        server_id(), crypto_config(),
        &push_promise_index_);
  }

  void QuicSpdyClientBase::SendRequest(const SpdyHeaderBlock& headers,
      QuicStringPiece body,
      bool fin) {
    QuicClientPushPromiseIndex::TryHandle* handle;
    QuicAsyncStatus rv = push_promise_index()->Try(headers, this, &handle);
    // std::cout << "SendRequest" << std::endl;
    if (rv == QUIC_SUCCESS){
      std::cout << "QUICSUCCESS" << std::endl;
      return;
    }

    if (rv == QUIC_PENDING) {
      // May need to retry request if asynchronous rendezvous fails.
      std::cout << "QUIC_PENDING" << std::endl;
      AddPromiseDataToResend(headers, body, fin);
      return;
    }

    // std::cout << "QUIC" << rv << std::endl;
    QuicSpdyClientStream* stream = CreateClientStream();
    if (stream == nullptr) {
      QUIC_BUG << "stream creation failed!";
      return;
    }
    stream->SendRequest(headers.Clone(), body, fin);
    // Record this in case we need to resend.
    MaybeAddDataToResend(headers, body, fin);
  }

  bool QuicSpdyClientBase::SendRequestAndWaitForResponse(
      const SpdyHeaderBlock& headers,
      QuicStringPiece body,
      bool fin) {
    SendRequest(headers, body, fin);
    return WaitForEvents();
  }

  void QuicSpdyClientBase::SendRequestsAndWaitForResponse(
      const std::vector<string>& url_list) {
    for (size_t i = 0; i < url_list.size(); ++i) {
      SpdyHeaderBlock headers;
      if (!SpdyUtils::PopulateHeaderBlockFromUrl(url_list[i], &headers)) {
        QUIC_BUG << "Unable to create request";
        continue;
      }
      SendRequest(headers, "", true);
    }
    while (WaitForEvents()) {
    }
  }

  QuicSpdyClientStream* QuicSpdyClientBase::CreateClientStream() {
    if (!connected()) {
      return nullptr;
    }

    auto* stream = static_cast<QuicSpdyClientStream*>(
        FLAGS_quic_reloadable_flag_quic_refactor_stream_creation
        ? client_session()->MaybeCreateOutgoingDynamicStream(kDefaultPriority)
        : client_session()->CreateOutgoingDynamicStream(kDefaultPriority));
    if (stream) {
      stream->set_visitor(this);
    }
    return stream;
  }

  int QuicSpdyClientBase::GetNumSentClientHellosFromSession() {
    return client_session()->GetNumSentClientHellos();
  }

  int QuicSpdyClientBase::GetNumReceivedServerConfigUpdatesFromSession() {
    return client_session()->GetNumReceivedServerConfigUpdates();
  }

  void QuicSpdyClientBase::MaybeAddDataToResend(const SpdyHeaderBlock& headers,
      QuicStringPiece body,
      bool fin) {
    if (!FLAGS_quic_reloadable_flag_enable_quic_stateless_reject_support) {
      return;
    }

    if (client_session()->IsCryptoHandshakeConfirmed()) {
      // The handshake is confirmed.  No need to continue saving requests to
      // resend.
      data_to_resend_on_connect_.clear();
      return;
    }

    // The handshake is not confirmed.  Push the data onto the queue of data to
    // resend if statelessly rejected.
    std::unique_ptr<SpdyHeaderBlock> new_headers(
        new SpdyHeaderBlock(headers.Clone()));
    std::unique_ptr<QuicDataToResend> data_to_resend(
        new ClientQuicDataToResend(std::move(new_headers), body, fin, this));
    MaybeAddQuicDataToResend(std::move(data_to_resend));
  }

  void QuicSpdyClientBase::MaybeAddQuicDataToResend(
      std::unique_ptr<QuicDataToResend> data_to_resend) {
    data_to_resend_on_connect_.push_back(std::move(data_to_resend));
  }

  void QuicSpdyClientBase::ClearDataToResend() {
    data_to_resend_on_connect_.clear();
  }

  void QuicSpdyClientBase::ResendSavedData() {
    // Calling Resend will re-enqueue the data, so swap out
    //  data_to_resend_on_connect_ before iterating.
    std::vector<std::unique_ptr<QuicDataToResend>> old_data;
    old_data.swap(data_to_resend_on_connect_);
    for (const auto& data : old_data) {
      data->Resend();
    }
  }

  void QuicSpdyClientBase::AddPromiseDataToResend(const SpdyHeaderBlock& headers,
      QuicStringPiece body,
      bool fin) {
    std::unique_ptr<SpdyHeaderBlock> new_headers(
        new SpdyHeaderBlock(headers.Clone()));
    push_promise_data_to_resend_.reset(
        new ClientQuicDataToResend(std::move(new_headers), body, fin, this));
  }

  bool QuicSpdyClientBase::CheckVary(const SpdyHeaderBlock& client_request,
      const SpdyHeaderBlock& promise_request,
      const SpdyHeaderBlock& promise_response) {
    return true;
  }

  void QuicSpdyClientBase::OnRendezvousResult(QuicSpdyStream* stream) {
    std::unique_ptr<ClientQuicDataToResend> data_to_resend =
      std::move(push_promise_data_to_resend_);
    if (stream) {
      stream->set_visitor(this);
      stream->OnDataAvailable();
    } else if (data_to_resend) {
      data_to_resend->Resend();
    }
  }

  size_t QuicSpdyClientBase::latest_response_code() const {
    QUIC_BUG_IF(!store_response_) << "Response not stored!";
    return latest_response_code_;
  }

  const string& QuicSpdyClientBase::latest_response_headers() const {
    QUIC_BUG_IF(!store_response_) << "Response not stored!";
    return latest_response_headers_;
  }

  const string& QuicSpdyClientBase::preliminary_response_headers() const {
    QUIC_BUG_IF(!store_response_) << "Response not stored!";
    return preliminary_response_headers_;
  }

  const SpdyHeaderBlock& QuicSpdyClientBase::latest_response_header_block()
    const {
      QUIC_BUG_IF(!store_response_) << "Response not stored!";
      return latest_response_header_block_;
    }

  const string& QuicSpdyClientBase::latest_response_body() const {
    QUIC_BUG_IF(!store_response_) << "Response not stored!";
    return latest_response_body_;
  }

  const string& QuicSpdyClientBase::latest_response_trailers() const {
    QUIC_BUG_IF(!store_response_) << "Response not stored!";
    return latest_response_trailers_;
  }

}  // namespace net
