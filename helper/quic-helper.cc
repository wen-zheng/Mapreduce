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

#include "ns3/log.h"
#include "ns3/names.h"
#include "ns3/node.h"
#include "ns3/node-list.h"
#include "quic-helper.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("QuicHelper");

QuicHelper::QuicHelper ()
{
  SetQuic ("ns3::QuicL4Protocol");
}

QuicHelper::~QuicHelper ()
{
}

void
QuicHelper::SetQuic (const std::string tid)
{
  m_quicFactory.SetTypeId (tid);
}

void
QuicHelper::Install (NodeContainer c) const
{
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      Install (*i);
    }
}

void
QuicHelper::InstallAll (void) const
{
  Install (NodeContainer::GetGlobal ());
}

void
QuicHelper::Install (Ptr<Node> node) const
{
  node->AggregateObject (m_quicFactory.Create<Object> ());
}

void
QuicHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  Install (node);
}

} // namespace ns3
