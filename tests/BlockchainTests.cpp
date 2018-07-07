#include "BlockchainTests.h"

#include "blockchain.h"
#include "consensus/regtest.h"

CPPUNIT_TEST_SUITE_REGISTRATION(BlockchainTest);

BlockchainTest::BlockchainTest() {
    log.reset(new CryptoKernel::Log("tests.log"));
}

BlockchainTest::~BlockchainTest() {
    CryptoKernel::Storage::destroy("./testblockdb");
}

void BlockchainTest::setUp() {
    std::remove("genesistest.json");
    CryptoKernel::Storage::destroy("./testblockdb");

    blockchain.reset(new testChain(log.get()));
    consensus.reset(new CryptoKernel::Consensus::Regtest(blockchain.get()));
    blockchain->loadChain(consensus.get(), "genesistest.json");
    consensus->start();
}

void BlockchainTest::tearDown() {}

BlockchainTest::testChain::testChain(CryptoKernel::Log* GlobalLog) : CryptoKernel::Blockchain(GlobalLog, "./testblockdb") {}

BlockchainTest::testChain::~testChain() {}

std::string BlockchainTest::testChain::getCoinbaseOwner(const std::string& publicKey) {
    return publicKey;
}

uint64_t BlockchainTest::testChain::getBlockReward(const uint64_t height) {
    return 100000000;
}

void BlockchainTest::testVerifyMalformedSignature() {
    consensus->mineBlock(true, "BL2AcSzFw2+rGgQwJ25r7v/misIvr3t4JzkH3U1CCknchfkncSneKLBo6tjnKDhDxZUSPXEKMDtTU/YsvkwxJR8=");

    const auto block = blockchain->getBlockByHeight(2);

    const auto output = *block.getCoinbaseTx().getOutputs().begin();

    CryptoKernel::Blockchain::output outp(output.getValue() - 20000, 0, Json::nullValue);

    Json::Value spendData;
    Json::Value randomData;
    randomData["this is"] = "malformed";

    // Something malformed (not a string)
    spendData["signature"] = randomData;

    CryptoKernel::Blockchain::input inp(output.getId(), spendData);

    CryptoKernel::Blockchain::transaction tx({inp}, {outp}, 1530888581);

    const auto res = blockchain->submitTransaction(tx);

    CPPUNIT_ASSERT(!std::get<0>(res));
}
