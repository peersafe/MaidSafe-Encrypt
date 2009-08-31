/*
* ============================================================================
*
* Copyright [2009] maidsafe.net limited
*
* Description:  Class containing vault logic
* Version:      1.0
* Created:      2009-02-21-23.55.54
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

#ifndef MAIDSAFE_VAULT_PDVAULT_H_
#define MAIDSAFE_VAULT_PDVAULT_H_

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <maidsafe/crypto.h>
#include <maidsafe/maidsafe-dht.h>
#include <maidsafe/utils.h>

#include <string>
#include <list>
#include <vector>

#include "boost/threadpool.hpp"  // NB - This is NOT an accepted boost lib.
#include "maidsafe/vault/vaultchunkstore.h"
#include "maidsafe/vault/vaultrpc.h"
#include "maidsafe/vault/vaultservice.h"

// This forward declaration is to allow gtest environment to be declared as a
// friend class
namespace localvaults {
class Env;
}  // namespace localvaults

namespace maidsafe_vault {

// Tuple of pmid, non-hex chunkname, chunksize, and pmid_publickey in that order
typedef boost::tuple<std::string, std::string, boost::uint64_t,
                     std::string> IouReadyTuple;

enum VaultStatus {kVaultStarted, kVaultStopping, kVaultStopped};

class KadCallback {
 public:
  KadCallback() : response_(""), mutex_() {}
  ~KadCallback() {}
  void SetResponse(const std::string &response) {
    boost::mutex::scoped_lock lock(mutex_);
    response_ = response;
  }
  std::string response() {
    boost::mutex::scoped_lock lock(mutex_);
    return response_;
  }
 private:
  KadCallback(const KadCallback&);
  KadCallback& operator=(const KadCallback&);
  std::string response_;
  boost::mutex mutex_;
};

struct StoreRefResultHolder {
  StoreRefResultHolder()
      : store_ref_response_(),
        store_ref_response_returned_(false),
        controller_(new rpcprotocol::Controller) {}
  maidsafe::StoreReferenceResponse store_ref_response_;
  bool store_ref_response_returned_;
  boost::shared_ptr<rpcprotocol::Controller> controller_;
};

struct SyncVaultData {
  SyncVaultData() : chunk_names(), num_updated_chunks(0), num_chunks(0),
    active_updating(0), cb(), is_callbacked(false) {}
  std::list<std::string> chunk_names;  // chunks to be updated
  int num_updated_chunks;  // number of chunks updated
  int num_chunks;
  int active_updating;
  base::callback_func_type cb;
  bool is_callbacked;
};

struct GetAlivePartner {
  GetAlivePartner(const int &num_parners, const std::string &chunk_name) :
    data(), is_found(false), contacted_partners(0),
    number_partners(num_parners), chunk_name(chunk_name) {}
  boost::shared_ptr<SyncVaultData> data;
  bool is_found;
  int contacted_partners;
  int number_partners;
  std::string chunk_name;
};

struct RepublishChunkRefData {
  RepublishChunkRefData() : chunk_names(), num_republished_chunks(0),
    num_chunks(0), cb(), is_callbacked(false) {}
  std::list<std::string> chunk_names;  // chunks to be updated
  int num_republished_chunks;  // number of chunks updated
  int num_chunks;
  base::callback_func_type cb;
  bool is_callbacked;
};

struct LoadChunkData {
  LoadChunkData(const std::string &chunkname, base::callback_func_type cb)
    : chunk_holders(), failed_chunk_holders(), number_holders(0),
      failed_holders(0), is_active(false), chunk_name(chunkname), retry(0),
      cb(cb), is_callbacked(false), get_msgs(false), pub_key(""),
      sig_pub_key("") {}
  std::vector<kad::Contact> chunk_holders;
  std::vector<kad::Contact> failed_chunk_holders;
  int number_holders;
  int failed_holders;
  bool is_active;
  std::string chunk_name;
  int retry;
  base::callback_func_type cb;
  bool is_callbacked;
  // used only for get msgs
  bool get_msgs;
  std::string pub_key;
  std::string sig_pub_key;
};

struct SynchArgs {
  SynchArgs(const std::string &chunk_name,
            const kad::Contact &chunk_holder,
            boost::shared_ptr<SyncVaultData> data)
                : chunk_name_(chunk_name),
                  chunk_holder_(chunk_holder),
                  data_(data) {}
  const std::string chunk_name_;
  const kad::Contact chunk_holder_;
  boost::shared_ptr<SyncVaultData> data_;
};

struct ValidityCheckArgs {
  ValidityCheckArgs(const std::string &chunk_name,
                    const std::string &random_data,
                    const kad::Contact &chunk_holder,
                    base::callback_func_type cb)
                        : chunk_name_(chunk_name),
                          random_data_(random_data),
                          chunk_holder_(chunk_holder),
                          cb_(cb), retry_remote(false) {}
  const std::string chunk_name_;
  const std::string random_data_;
  const kad::Contact chunk_holder_;
  base::callback_func_type cb_;
  bool retry_remote;
};

struct GetArgs {
  GetArgs(const kad::Contact &chunk_holder,
          boost::shared_ptr<struct LoadChunkData> data)
      : chunk_holder_(chunk_holder),
        data_(data),
        retry_remote_(false),
        controller_(new rpcprotocol::Controller) {}
  const kad::Contact chunk_holder_;
  boost::shared_ptr<struct LoadChunkData> data_;
  bool retry_remote_;
  boost::shared_ptr<rpcprotocol::Controller> controller_;
};

struct SwapChunkArgs {
  SwapChunkArgs(const std::string &chunkname,
                const std::string &remote_ip,
                const boost::uint16_t &remote_port,
                const std::string &rendezvous_ip,
                const boost::uint16_t &rendezvous_port,
                base::callback_func_type cb)
     : chunkname_(chunkname),
       remote_ip_(remote_ip),
       remote_port_(remote_port),
       rendezvous_ip_(rendezvous_ip),
       rendezvous_port_(rendezvous_port),
       cb_(cb) {}
  std::string chunkname_;
  std::string remote_ip_;
  boost::uint16_t remote_port_;
  std::string rendezvous_ip_;
  boost::uint16_t rendezvous_port_;
  base::callback_func_type cb_;
};

class PDVault {
 public:
  PDVault(const std::string &pmid_public,
          const std::string &pmid_private,
          const std::string &signed_pmid_public,
          const std::string &chunkstore_dir,
          const boost::uint16_t &port,
          const std::string &kad_config_file,
          const boost::uint64_t &available_space,
          const boost::uint64_t &vault_used_space);
  ~PDVault();
  void Start(bool port_forwarded);
  int Stop(bool cancel_pending_ops);
  void CleanUp();
  VaultStatus vault_status();
  void SetVaultStatus(const VaultStatus &vault_status);
  std::string hex_node_id() const;
  std::string host_ip() const { return knode_.host_ip(); }
  boost::uint16_t host_port() const { return knode_.host_port(); }
  std::string local_host_ip() const { return knode_.local_host_ip(); }
  boost::uint16_t local_host_port() { return knode_.local_host_port(); }
  std::string rv_ip() const { return knode_.rv_ip(); }
  boost::uint16_t rv_port() const { return knode_.rv_port(); }
  inline boost::uint64_t available_space() {
    return vault_chunkstore_.available_space();
  }
  inline boost::uint64_t UsedSpace() { return vault_chunkstore_.used_space(); }
  inline boost::uint64_t FreeSpace() { return vault_chunkstore_.FreeSpace(); }




  void SyncVault(base::callback_func_type cb);
  void RepublishChunkRef(base::callback_func_type cb);
  void ValidityCheck(const std::string &chunk_name,
                     const std::string &random_data,
                     const kad::Contact &remote,
                     int attempt,
                     base::callback_func_type cb);
  void GetChunk(const std::string &chunk_name, base::callback_func_type cb);
  void SwapChunk(const std::string &chunk_name,
                 const std::string &remote_ip,
                 const boost::uint16_t &remote_port,
                 const std::string &rendezvous_ip,
                 const boost::uint16_t &rendezvous_port,
                 base::callback_func_type cb);
  friend class localvaults::Env;
 private:
  PDVault(const PDVault&);
  PDVault& operator=(const PDVault&);
  FRIEND_TEST(TestPDVault, FUNC_MAID_StoreChunks);
  void KadJoinedCallback(const std::string &result,
                         boost::mutex *mutex,
                         boost::condition_variable *kad_join_cond);
  void RegisterMaidService();
  void UnRegisterMaidService();
  // This runs in a continuous loop until vault_status_ becomes kVaultStopping.
  bool PrunePendingOperations();
  // This runs in a continuous loop until vault_status_ becomes kVaultStopping
  // and on receipt of an IOU_Ready messgage, it adds an AddToRefPacket task.
  bool CheckPendingIOUs();
  // Returns a signature for validation by recipient of RPC
  std::string GetSignedRequest(const std::string &non_hex_name,
                               const std::string &recipient_id);
  // Runs in a worker thread to add this vault's ID to a chunk reference packet
  // and increment this vault's rank.
  void AddToRefPacket(const IouReadyTuple &iou_ready_details);
  // Finds k closest nodes to the kad_key
  int FindKNodes(const std::string &kad_key,
                 std::vector<kad::Contact> *contacts);
  // Add this vault's ID to a chunk reference packet on a
  int SendToRefPacket(
      const kad::Contact &ref_holder,
      const IouReadyTuple &iou_ready_details,
      boost::mutex *store_ref_mutex,
      StoreRefResultHolder *store_ref_result_holder);
  void SendToRefPacketCallback(StoreRefResultHolder *store_ref_result_holder,
                               boost::mutex *store_ref_mutex);
  int HandleStoreRefResponse(const IouReadyTuple &iou_ready_details,
      const StoreRefResultHolder &store_ref_result_holder,
      bool *got_valid_iou);



  void IterativeSyncVault(boost::shared_ptr<SyncVaultData> data);
  void SyncVault_FindAlivePartner(
      const std::string& result,
      boost::shared_ptr<SyncVaultData> data,
      std::string chunk_name);
  void SyncVault_FindAlivePartner_Callback(
      const std::string& result,
      boost::shared_ptr<GetAlivePartner> partner_data,
      kad::Contact remote);
  void ValidityCheckCallback(
    boost::shared_ptr<maidsafe::ValidityCheckResponse> validity_check_response,
    boost::shared_ptr<ValidityCheckArgs> validity_check_args);
  void IterativeSyncVault_SyncChunk(
      const std::string& result,
      boost::shared_ptr<SyncVaultData> data,
      std::string chunk_name, kad::Contact remote);
  void IterativeSyncVault_UpdateChunk(
      boost::shared_ptr<maidsafe::GetResponse> get_chunk_response,
      boost::shared_ptr<SynchArgs> synch_args);
  void IterativePublishChunkRef(
      boost::shared_ptr<RepublishChunkRefData> data);
  void IterativePublishChunkRef_Next(const std::string &result,
      boost::shared_ptr<RepublishChunkRefData> data);
  void CheckChunk(boost::shared_ptr<GetArgs> get_args);
  void CheckChunkCallback(boost::shared_ptr<maidsafe::CheckChunkResponse>
      check_chunk_response, boost::shared_ptr<GetArgs> get_args);
  void GetMessagesCallback(boost::shared_ptr<maidsafe::GetMessagesResponse>
      get_messages_response, boost::shared_ptr<GetArgs> get_args);
  void GetChunkCallback(boost::shared_ptr<maidsafe::GetResponse> get_response,
                        boost::shared_ptr<GetArgs> get_args);
  void RetryGetChunk(boost::shared_ptr<struct LoadChunkData> data);
  void FindChunkRef(boost::shared_ptr<struct LoadChunkData> data);
  void FindChunkRefCallback(const std::string &result,
                            boost::shared_ptr<struct LoadChunkData> data);
  void GetMessages(const std::string &chunk_name,
                   const std::string &public_key,
                   const std::string &signed_public_key,
                   base::callback_func_type cb);
  void SwapChunkSendChunk(
      boost::shared_ptr<maidsafe::SwapChunkResponse> swap_chunk_response,
      boost::shared_ptr<SwapChunkArgs> swap_chunk_args);
  void SwapChunkAcceptChunk(
      boost::shared_ptr<maidsafe::SwapChunkResponse> swap_chunk_response,
      boost::shared_ptr<SwapChunkArgs> swap_chunk_args);
  void StopRvPing() { knode_.StopRvPing(); }
  boost::uint16_t port_;
  boost::shared_ptr<rpcprotocol::ChannelManager> channel_manager_;
  kad::KNode knode_;
  VaultRpcs vault_rpcs_;
  VaultChunkStore vault_chunkstore_;
  boost::shared_ptr<VaultService> vault_service_;
  bool kad_joined_;
  VaultStatus vault_status_;
  boost::mutex vault_status_mutex_;
  std::string pmid_public_, pmid_private_, signed_pmid_public_, pmid_;
  std::string non_hex_pmid_, signed_non_hex_pmid_;
  crypto::Crypto co_;
  boost::shared_ptr<rpcprotocol::Channel> svc_channel_;
  std::string kad_config_file_;
  PendingOperationsHandler poh_;
  boost::threadpool::thread_pool<
      boost::threadpool::task_func,
      boost::threadpool::fifo_scheduler,
      boost::threadpool::static_size,
      boost::threadpool::resize_controller,
      boost::threadpool::immediately> thread_pool_;
  const boost::uint16_t kKadStoreThreshold_;
};

}  // namespace maidsafe_vault

#endif  // MAIDSAFE_VAULT_PDVAULT_H_
