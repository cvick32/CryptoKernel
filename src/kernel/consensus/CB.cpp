#include <sstream>
#include <math.h>

#include "CB.h"
#include "../crypto.h"

CryptoKernel::Consensus::CB::PoW(CryptoKernel::Blockchain* blockchain,
                                  const std::string& pubKey,
                                  CryptoKernel::Log* log) {
    this->blockchain = blockchain;
    running = miner;
    this->pubKey = pubKey;
    this->log = log;
}

CryptoKernel::Consensus::CB::~PoW() {
    running = false;
    cbThread->join();
}

void CryptoKernel::Consensus::CB::start() {
    cbThread.reset(new std::thread(&CryptoKernel::Consensus::CB::miner, this));
}

bool CryptoKernel::Consensus::CB::isBlockBetter(Storage::Transaction* transaction,
        const CryptoKernel::Blockchain::block& block,
        const CryptoKernel::Blockchain::dbBlock& tip) {
    
    const consensusData blockData = getConsensusData(block);
    const consensusData tipData = getConsensusData(tip);
    CryptoKernel::Crypto crypto;
    crypto.setPublicKey(cbPubkey);
    return blockData.totalWork > tipData.totalWork && crypto.verify(block.getId().toString(), blockData.signature);
}

CryptoKernel::Consensus::CB::consensusData
CryptoKernel::Consensus::CB::getConsensusData(const CryptoKernel::Blockchain::block&
        block) {
    consensusData returning;
    const JSON::Value data = block.getConsensusData();
    returning.publicKey = data["publicKey"].asString();
    returning.signature = data["signature"].asString();
    return returning;
}

Json::Value CryptoKernel::Consensus::CB::consensusDataToJson(const
        CryptoKernel::Consensus:CB::consensusData& data) {
    Json::Value returning;
    returning["publicKey"] = data.publicKey;
    returning["signature"] = data.signature;
    returning["sequenceNumber"] = data.sequenceNumber;
    return returning;
}

bool CryptoKernel::Consensus::CB::checkConsensusRules(Storage::Transaction* transaction,
        CryptoKernel::Blockchain::block& block,
        const CryptoKernel::Blockchain::dbBlock& previousBlock) {
    
}

Json::Value CryptoKernel::Consensus::CB::generateConsensusData(
    Storage::Transaction* transaction, const CryptoKernel::BigNum& previousBlockId,
    const std::string& publicKey) {
    consensusData data;
    data.target = calculateTarget(transaction, previousBlockId);

    return consensusDataToJson(data);
}

CryptoKernel::Consensus::CB::KGW_SHA256::KGW_SHA256(const uint64_t blockTarget,
                                                     CryptoKernel::Blockchain* blockchain,
                                                     const bool miner,
                                                     const std::string& pubKey,
                                                     CryptoKernel::Log* log) :
CryptoKernel::Consensus::CB(blockTarget, blockchain, miner, pubKey, log) {

}

CryptoKernel::BigNum CryptoKernel::Consensus::CB::KGW_SHA256::powFunction(
    const std::string& inputString) {
    CryptoKernel::Crypto crypto;
    return CryptoKernel::BigNum(crypto.sha256(inputString));
}

CryptoKernel::BigNum CryptoKernel::Consensus::CB::KGW_SHA256::calculateTarget(
    Storage::Transaction* transaction, const CryptoKernel::BigNum& previousBlockId) {
    const uint64_t minBlocks = 144;
    const uint64_t maxBlocks = 4032;
    const CryptoKernel::BigNum minDifficulty =
        CryptoKernel::BigNum("fffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");

    CryptoKernel::Blockchain::dbBlock currentBlock = blockchain->getBlockDB(transaction,
            previousBlockId.toString());
    consensusData currentBlockData = getConsensusData(currentBlock);
    CryptoKernel::Blockchain::dbBlock lastSolved = currentBlock;

    if(currentBlock.getHeight() < minBlocks) {
        return minDifficulty;
    } else if(currentBlock.getHeight() % 12 != 0) {
        return currentBlockData.target;
    } else {
        uint64_t blocksScanned = 0;
        CryptoKernel::BigNum difficultyAverage = CryptoKernel::BigNum("0");
        CryptoKernel::BigNum previousDifficultyAverage = CryptoKernel::BigNum("0");
        int64_t actualRate = 0;
        int64_t targetRate = 0;
        double rateAdjustmentRatio = 1.0;
        double eventHorizonDeviation = 0.0;
        double eventHorizonDeviationFast = 0.0;
        double eventHorizonDeviationSlow = 0.0;

        for(unsigned int i = 1; currentBlock.getHeight() != 1; i++) {
            if(i > maxBlocks) {
                break;
            }

            blocksScanned++;

            if(i == 1) {
                difficultyAverage = currentBlockData.target;
            } else {
                std::stringstream buffer;
                buffer << std::hex << i;
                difficultyAverage = ((currentBlockData.target - previousDifficultyAverage) /
                                     CryptoKernel::BigNum(buffer.str())) + previousDifficultyAverage;
            }

            previousDifficultyAverage = difficultyAverage;

            actualRate = lastSolved.getTimestamp() - currentBlock.getTimestamp();
            targetRate = blockTarget * blocksScanned;
            rateAdjustmentRatio = 1.0;

            if(actualRate < 0) {
                actualRate = 0;
            }

            if(actualRate != 0 && targetRate != 0) {
                rateAdjustmentRatio = double(targetRate) / double(actualRate);
            }

            eventHorizonDeviation = 1 + (0.7084 * pow((double(blocksScanned)/double(minBlocks)),
                                         -1.228));
            eventHorizonDeviationFast = eventHorizonDeviation;
            eventHorizonDeviationSlow = 1 / eventHorizonDeviation;

            if(blocksScanned >= minBlocks) {
                if((rateAdjustmentRatio <= eventHorizonDeviationSlow) ||
                        (rateAdjustmentRatio >= eventHorizonDeviationFast)) {
                    break;
                }
            }

            if(currentBlock.getHeight() == 1) {
                break;
            }
            currentBlock = blockchain->getBlockDB(transaction,
                                                  currentBlock.getPreviousBlockId().toString());
            currentBlockData = getConsensusData(currentBlock);
        }

        CryptoKernel::BigNum newTarget = difficultyAverage;
        if(actualRate != 0 && targetRate != 0) {
            std::stringstream buffer;
            buffer << std::hex << actualRate;
            newTarget = newTarget * CryptoKernel::BigNum(buffer.str());

            buffer.str("");
            buffer << std::hex << targetRate;
            newTarget = newTarget / CryptoKernel::BigNum(buffer.str());
        }

        if(newTarget > minDifficulty) {
            newTarget = minDifficulty;
        }

        return newTarget;
    }
}

bool CryptoKernel::Consensus::CB::KGW_SHA256::verifyTransaction(
    Storage::Transaction* transaction, const CryptoKernel::Blockchain::transaction& tx) {
    return true;
}

bool CryptoKernel::Consensus::CB::KGW_SHA256::confirmTransaction(
    Storage::Transaction* transaction, const CryptoKernel::Blockchain::transaction& tx) {
    return true;
}

bool CryptoKernel::Consensus::CB::KGW_SHA256::submitTransaction(
    Storage::Transaction* transaction, const CryptoKernel::Blockchain::transaction& tx) {
    return true;
}

bool CryptoKernel::Consensus::CB::KGW_SHA256::submitBlock(Storage::Transaction*
        transaction, const CryptoKernel::Blockchain::block& block) {
    return true;
}

CryptoKernel::Consensus::CB::KGW_LYRA2REV2::KGW_LYRA2REV2(const uint64_t blockTarget,
                                                           CryptoKernel::Blockchain* blockchain,
                                                           const bool miner,
                                                           const std::string& pubKey,
                                                           CryptoKernel::Log* log)
: KGW_SHA256(blockTarget, blockchain, miner, pubKey, log) {}
