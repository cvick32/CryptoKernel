#include <sstream>
#include <math.h>

#include "CB.h"

CryptoKernel::Consensus::CB::CB(CryptoKernel::Blockchain* blockchain,
                                  const std::string& pubKey,
                                  CryptoKernel::Log* log) {
    this->blockchain = blockchain;
    centralBank = false;
    this->pubKey = pubKey;
    this->log = log;
    password = "froogy45";
    wallet == nullptr;
    crypto = new CryptoKernel::Crypto(true);
}

CryptoKernel::Consensus::CB::~CB() {
  centralBank = false;
  cbThread->join();
}

void CryptoKernel::Consensus::CB::setWallet(CryptoKernel::Wallet* Wallet) {
  wallet = Wallet;
  log->printf(LOG_LEVEL_INFO, "Consensus::CB::setWallet(): set wallet");
}

void CryptoKernel::Consensus::CB::checkCB() {
  Json::Value cdata = blockchain->getBlockByHeight(1).toJson();
  // get cbPubKey from genesis block
  this->cbPubKey = blockchain->getBlockByHeight(1).getConsensusData()["publicKey"].asString();
  std::string cbSignature = blockchain->getBlockByHeight(1).getConsensusData()["signature"].asString();

  Json::FastWriter fastWrite;
  std::string cdataout = fastWrite.write(cdata);
  std::string genesisBlockId = blockchain->getBlockByHeight(1).getId().toString();
  log->printf(LOG_LEVEL_INFO, "Consensus::CB::checkCB(): consensus data: " + cdataout);
  log->printf(LOG_LEVEL_INFO, "Consensus::CB::checkCB(): public key: " + pubKey);
  log->printf(LOG_LEVEL_INFO, "Consensus::CB::checkCB(): cb public key: " + cbPubKey);
  log->printf(LOG_LEVEL_INFO, "Consensus::CB::checkCB(): genesisBlockId: " + genesisBlockId);
  log->printf(LOG_LEVEL_INFO, "Consensus::CB::checkCB(): cbSignature: " + cbSignature);

  if (crypto->verify(genesisBlockId, cbSignature)) {
    centralBank = true;
    log->printf(LOG_LEVEL_INFO, "Consensus::CB::checkCB(): you are the central bank!");
  } else {
    centralBank = false;
    log->printf(LOG_LEVEL_INFO, "Consensus::CB::checkCB(): you are not the central bank!");
  }
}

void CryptoKernel::Consensus::CB::start() {
  checkCB();
  cbThread.reset(new std::thread(&CryptoKernel::Consensus::CB::centralBanker, this));
}

void CryptoKernel::Consensus::CB::centralBanker() {
  while(centralBank) {
    log->printf(LOG_LEVEL_INFO, "Consensus::CB::centralBanker(): looking for unconfirmed transactions");
    std::set<CryptoKernel::Blockchain::transaction> uctxs = blockchain->getUnconfirmedTransactions();

    if (!uctxs.empty()) {
      // sign the block with our pubkey
      CryptoKernel::Blockchain::block Block = blockchain->generateVerifyingBlock(pubKey);
      std::string blockId = Block.getId().toString();
      std::string signature = wallet->signMessage(blockId, pubKey, password);
      
      // create consensus data
      Json::Value consensusData = Block.getConsensusData(); 
      consensusData["signature"] = signature;
      consensusData["publicKey"] = pubKey;

      Block.setConsensusData(consensusData);

      log->printf(LOG_LEVEL_WARN, "Consensus::CB::centralBanker(): signature = " + consensusData["signature"].asString());
      log->printf(LOG_LEVEL_WARN, "Consensus::CB::centralBanker(): public key = " + consensusData["publicKey"].asString());

      const auto res = blockchain->submitBlock(Block);
      
      if(!std::get<0>(res)) {
        log->printf(LOG_LEVEL_WARN, "Consensus::CB::centralBanker(): minted block was rejected by blockchain");
      } else {
        log->printf(LOG_LEVEL_INFO, "Consensus::CB::centralBanker(): minted a block! Submitting to blockchain");
      }
    } else {
      log->printf(LOG_LEVEL_INFO, "Consensus::CB::centralBanker(): no unconfirmed transactions");
      std::this_thread::sleep_for(std::chrono::seconds(5)); // don't look for 5 seconds    
    }
  }
}

bool CryptoKernel::Consensus::CB::isBlockBetter(Storage::Transaction* transaction,
        const CryptoKernel::Blockchain::block& block,
        const CryptoKernel::Blockchain::dbBlock& tip) {
    return false;
}

CryptoKernel::Consensus::CB::consensusData
CryptoKernel::Consensus::CB::getConsensusData(const CryptoKernel::Blockchain::block&
        block) {
    consensusData returning;
    const Json::Value data = block.getConsensusData();
    returning.publicKey = data["publicKey"].asString();
    returning.signature = data["signature"].asString();
    return returning;
}

CryptoKernel::Consensus::CB::consensusData
CryptoKernel::Consensus::CB::getConsensusData(const CryptoKernel::Blockchain::dbBlock&
        block) {
    consensusData returning;
    const Json::Value data = block.getConsensusData();
    returning.publicKey = data["publicKey"].asString();
    returning.signature = data["signature"].asString();
    return returning;
}

Json::Value CryptoKernel::Consensus::CB::consensusDataToJson(const
        CryptoKernel::Consensus::CB::consensusData& data) {
    Json::Value returning;
    returning["publicKey"] = data.publicKey;
    returning["signature"] = data.signature;
    return returning;
}

bool CryptoKernel::Consensus::CB::checkConsensusRules(Storage::Transaction* transaction,
        CryptoKernel::Blockchain::block& block,
        const CryptoKernel::Blockchain::dbBlock& previousBlock) {
    // did the central bank sign this block?
    return true;
}

Json::Value CryptoKernel::Consensus::CB::generateConsensusData(
    Storage::Transaction* transaction, const CryptoKernel::BigNum& previousBlockId,
    const std::string& publicKey) {
    consensusData data;
    data.publicKey = publicKey;

    return consensusDataToJson(data);
}

bool CryptoKernel::Consensus::CB::verifyTransaction(
    Storage::Transaction* transaction, const CryptoKernel::Blockchain::transaction& tx) {
    return true;
}

bool CryptoKernel::Consensus::CB::confirmTransaction(
    Storage::Transaction* transaction, const CryptoKernel::Blockchain::transaction& tx) {
    return true;
}

bool CryptoKernel::Consensus::CB::submitTransaction(
    Storage::Transaction* transaction, const CryptoKernel::Blockchain::transaction& tx) {
    return true;
}

bool CryptoKernel::Consensus::CB::submitBlock(Storage::Transaction*
        transaction, const CryptoKernel::Blockchain::block& block) {
    if (pubKey == cbPubKey) {
      CryptoKernel::Crypto crypto;
      return true;
    }
  return true;
}