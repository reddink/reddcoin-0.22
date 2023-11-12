// Copyright (c) 2011-2020 The Bitcoin Core developers
// Copyright (c) 2014-2023 The Reddcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <chainparams.h>
#include <consensus/consensus.h>
#include <consensus/params.h>
#include <consensus/validation.h>
#include <crypto/sha256.h>
#include <test/util/mining.h>
#include <test/util/script.h>
#include <test/util/setup_common.h>
#include <test/util/wallet.h>
#include <txmempool.h>
#include <validation.h>

#include <vector>

static void AssembleBlock(benchmark::Bench& bench)
{
    const CChainParams& chainparams = Params();
    const auto test_setup = MakeNoLogFileContext<const TestingSetup>();

    CScriptWitness witness;
    witness.stack.push_back(WITNESS_STACK_ELEM_OP_TRUE);

    // Collect some loose transactions that spend the coinbases of our mined blocks
    const int NUM_BLOCKS{200};
    const int COINBASE_MATURITY = chainparams.GetConsensus().GetCoinbaseMaturity();
    std::vector<CTransactionRef> txs;
    for (int b{0}; b < NUM_BLOCKS; ++b) {
        CMutableTransaction tx;
        tx.vin.push_back(MineBlock(test_setup->m_node, P2WSH_OP_TRUE));
        tx.vin.back().scriptWitness = witness;
        tx.vout.emplace_back(1337, P2WSH_OP_TRUE);
        if (NUM_BLOCKS - b >= COINBASE_MATURITY)
            txs.at(b) = MakeTransactionRef(tx);
    }
    {
        LOCK(::cs_main); // Required for ::AcceptToMemoryPool.

        for (const auto& txr : txs) {
            const MempoolAcceptResult res = ::AcceptToMemoryPool(test_setup->m_node.chainman->ActiveChainstate(), *test_setup->m_node.mempool, txr, false /* bypass_limits */);
            assert(res.m_result_type == MempoolAcceptResult::ResultType::VALID);
        }
    }

    bench.run([&] {
        PrepareBlock(test_setup->m_node, P2WSH_OP_TRUE);
    });
}

BENCHMARK(AssembleBlock);
