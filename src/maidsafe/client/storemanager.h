/*
* ============================================================================
*
* Copyright [2009] maidsafe.net limited
*
* Description:  Interface allowing storage of data to network or local database
* Version:      1.0
* Created:      2009-01-29-00.49.17
* Revision:     none
* Compiler:     gcc
* Author:       Fraser Hutchison (fh), fraser.hutchison@maidsafe.net
* Company:      maidsafe.net limited
*
* The following source code is property of maidsafe.net limited and is not
* meant for external use.  The use of this code is governed by the license
* file LICENSE.TXT found in the root of this directory and also on
* www.maidsafe.net.
*
* You are not free to copy, amend or otherwise use this source code without
* the explicit written permission of the board of directors of maidsafe.net.
*
* ============================================================================
*/

#ifndef MAIDSAFE_CLIENT_STOREMANAGER_H_
#define MAIDSAFE_CLIENT_STOREMANAGER_H_

#include <maidsafe/maidsafe-dht.h>
#include <maidsafe/utils.h>

#include <string>

#include "protobuf/maidsafe_service_messages.pb.h"
#include "maidsafe/maidsafe.h"
#include "maidsafe/client/packetfactory.h"

namespace maidsafe {

class StoreManagerInterface {
 public:
  virtual ~StoreManagerInterface() {}
  virtual void Init(int port, base::callback_func_type cb)=0;
  virtual void Close(base::callback_func_type cb, bool cancel_pending_ops)=0;
  virtual void CleanUpTransport()=0;
  virtual int LoadChunk(const std::string &hex_chunk_name, std::string *data)=0;
  virtual void StoreChunk(const std::string &hex_chunk_name,
                          const DirType dir_type,
                          const std::string &msid)=0;
  virtual int StorePacket(const std::string &hex_packet_name,
                          const std::string &value,
                          packethandler::SystemPackets system_packet_type,
                          DirType dir_type,
                          const std::string &msid)=0;
  virtual void IsKeyUnique(const std::string &hex_key,
                           base::callback_func_type cb)=0;
  virtual void DeletePacket(const std::string &hex_key,
                            const std::string &signature,
                            const std::string &public_key,
                            const std::string &signed_public_key,
                            const ValueType &type,
                            base::callback_func_type cb)=0;
  virtual void LoadPacket(const std::string &hex_key,
                          base::callback_func_type cb)=0;

  // The public_key is the one of the MPID and is signed by the MPID's
  // private key
  virtual void GetMessages(const std::string &hex_key,
                           const std::string &public_key,
                           const std::string &signed_public_key,
                           base::callback_func_type cb)=0;
  virtual void PollVaultInfo(base::callback_func_type cb)=0;
  virtual void VaultContactInfo(base::callback_func_type cb)=0;
  virtual void OwnLocalVault(const std::string &priv_key, const std::string
      &pub_key, const std::string &signed_pub_key, const boost::uint32_t &port,
      const std::string &chunkstore_dir, const boost::uint64_t &space,
      boost::function<void(const OwnVaultResult&, const std::string&)> cb)=0;
  virtual void LocalVaultStatus(boost::function<void(const VaultStatus&)> cb)=0;
};

}  // namespace maidsafe

#endif  // MAIDSAFE_CLIENT_STOREMANAGER_H_
