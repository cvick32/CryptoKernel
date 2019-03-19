#ifndef CB_H_INCLUDED
#define CB_H_INCLUDED

#include <thread>

#include "../blockchain.h"

namespace CryptoKernel {
/**
* Implements a Central Bank consensus algorithm. The Central Bank will verify
* all transactions and publish blocks.
*/
class Consensus::CB : public Consensus {
public:
    /**
    * Constructs a Proof of Work consensus object. Provides Bitcoin-style
    * Proof of Work to the blockchain module.
    *
    * @param blockTarget the target number of seconds per block
    * @param blockchain a pointer to the blockchain to be used with this
    *        consensus object
    * @param miner a flag to determine whether the consensus object should mine
    * @param pubKey if the miner is enabled, rewards will be sent to this pubKey
    */
    CB(CryptoKernel::Blockchain* blockchain,
        const std::string& pubKey,
        CryptoKernel::Log* log);

    virtual ~CB();

    /**
    * Just checks if the new block is higher than the current block.
    */
    bool isBlockBetter(Storage::Transaction* transaction,
                       const CryptoKernel::Blockchain::block& block,
                       const CryptoKernel::Blockchain::dbBlock& tip);

    /**
    * Checks the following rules:
    *   - check the block was signed by the Central Bank
    */
    bool checkConsensusRules(Storage::Transaction* transaction,
                             CryptoKernel::Blockchain::block& block,
                             const CryptoKernel::Blockchain::dbBlock& previousBlock);

    /**
    * Probably just the Central Bank pub key
    */
    Json::Value generateConsensusData(Storage::Transaction* transaction,
                                      const CryptoKernel::BigNum& previousBlockId, const std::string& publicKey);
 
    /**
    * Always return true.
    */ 
    virtual bool verifyTransaction(Storage::Transaction* transaction, 
                                  const CryptoKernel::Blockchain::transaction& tx);

    virtual bool confirmTransaction(Storage::Transaction* transaction,
                                    const CryptoKernel::Blockchain::transaction& tx);

    virtual bool submitTransaction(Storage::Transaction* transaction,
                                   const CryptoKernel::Blockchain::transaction& tx);
    
    /**
    * - have the Central Bank sign this block with its private key
    */ 
    virtual bool submitBlock(Storage::Transaction* transaction,
                             const CryptoKernel::Blockchain::block& block);

    virtual void start();
protected:
    CryptoKernel::Blockchain* blockchain;
    CryptoKernel::Log* log;
    uint64_t blockTarget;
    struct consensusData {
        BigNum totalWork;
        BigNum target;
        uint64_t nonce;
    };
    consensusData getConsensusData(const CryptoKernel::Blockchain::block& block);
    consensusData getConsensusData(const CryptoKernel::Blockchain::dbBlock& block);
    Json::Value consensusDataToJson(const consensusData& data);

private:  
    bool running;
    void miner();
    std::string pubKey;
    std::unique_ptr<std::thread> minerThread;
};

#endif // CB_H_INCLUDED