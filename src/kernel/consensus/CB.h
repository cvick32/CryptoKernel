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
    * Check if the block is higher and signed by the CB.
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
    * Probably just return the Central Bank pub key.
    */
    Json::Value generateConsensusData(Storage::Transaction* transaction,
                                      const CryptoKernel::BigNum& previousBlockId, const std::string& publicKey);
 
    /**
    * Always return true. No custom functionality.
    */ 
    virtual bool verifyTransaction(Storage::Transaction* transaction, 
                                  const CryptoKernel::Blockchain::transaction& tx);
    /**
    * Always return true. No custom functionality.
    */   
    virtual bool confirmTransaction(Storage::Transaction* transaction,
                                    const CryptoKernel::Blockchain::transaction& tx);
    /**
    * Always return true. No custom functionality. 
    */ 
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
    
    consensusData getConsensusData(const CryptoKernel::Blockchain::block& block);
    Json::Value consensusDataToJson(const consensusData& data);
private:  
    std::string pubkey;
    bool running;
    void centralBanker();
    std::string cbPubkey;
    struct consensusData {
        std::string cbPubkey;
    };
    std::unique_ptr<std::thread> cbThread;
};

}

#endif // CB_H_INCLUDED