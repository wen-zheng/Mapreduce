/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2017 CCSL IME-USP
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Diego Araújo <diegoamc@ime.usp.br>
 */

#ifndef QUIC_HELPER_H
#define QUIC_HELPER_H

#include "ns3/node-container.h"
#include "ns3/packet.h"
#include "ns3/ptr.h"
#include "ns3/object-factory.h"

namespace ns3 {

class Node;
/**
 * \ingroup quic
 *
 * \brief aggregate QUIC functionality to existing Nodes.
 *
 * This class aggregates an instance of a QUIC object based on the QUIC factory provided, by default, to each node.
 * This class assumes the node already has an InternetStack installed.
 */
class QuicHelper
{
public:
  /**
   * Create a new QuicHelper.
   */
  QuicHelper(void);

  /**
   * Destroy the QuicHelper
   */
  virtual ~QuicHelper(void);

  /**
   * Aggregate implementations of the ns3::Quic classes onto the provided node.
   *
   * \param nodeName The name of the node on which to install the stack.
   */
  void Install (std::string nodeName) const;

  /**
   * Aggregate implementations of the ns3::Quic classes onto the provided node.
   *
   * \param node The node on which to install the stack.
   */
  void Install (Ptr<Node> node) const;

  /**
   * For each node in the input container, aggregate implementations of the
   * ns3::Quic classes.
   *
   * \param c NodeContainer that holds the set of nodes on which to install the
   * new stacks.
   */
  void Install (NodeContainer c) const;

  /**
   * Aggregate QUIC stack to all nodes in the simulation
   */
  void InstallAll (void) const;

  /**
   * \brief set the QUIC stack which will not need any other parameter.
   *
   * This function sets up the quic stack to the given TypeId.
   *
   * \param tid the type id, typically it is set to  "ns3::QuicL4Protocol"
   */
  void SetQuic (std::string tid);

private:
  /**
   * \brief QUIC objects factory
   */
  ObjectFactory m_quicFactory;
};

} // namespace ns3

#endif /* QUIC_HELPER_H */
