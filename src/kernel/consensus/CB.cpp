#include <sstream>
#include <math.h>

#include "CB.h"
#include "../crypto.h"

CryptoKernel::Consensus::CB::CB(CryptoKernel::Blockchain* blockchain,
                                  const std::string& pubkey,
                                  CryptoKernel::Log* log) {
    this->blockchain = blockchain;
    running = true;
    this->pubkey = pubkey;
    this->log = log;
    this->cbPubkey = pubkey;
}

CryptoKernel::Consensus::CB::~CB() {
    running = false;
    cbThread->join();
}

void CryptoKernel::Consensus::CB::start() {
    cbThread.reset(new std::thread(&CryptoKernel::Consensus::CB::centralBanker, this));
}

void CryptoKernel::Consensus::CB::centralBanker() {
  while(running) {
    CryptoKernel::Blockchain::block Block = blockchain->generateVerifyingBlock(pubkey);
    const auto res = blockchain->submitBlock(Block);
    if(!std::get<0>(res)) {
      log->printf(LOG_LEVEL_WARN, "Consensus::CB::centralBanker(): mined block was rejected by blockchain");
    }
  }
}

bool CryptoKernel::Consensus::CB::isBlockBetter(Storage::Transaction* transaction,
        const CryptoKernel::Blockchain::block& block,
        const CryptoKernel::Blockchain::dbBlock& tip) {
    
    const consensusData blockData = getConsensusData(block);
    const consensusData tipData = getConsensusData(tip);
    CryptoKernel::Crypto crypto;
    crypto.setPublicKey(cbPubkey);
    return crypto.verify(block.getId().toString(), blockData.signature);
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
    return true;
}