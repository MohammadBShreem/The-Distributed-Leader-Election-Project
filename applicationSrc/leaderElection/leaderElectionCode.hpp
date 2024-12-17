#ifndef LeaderElectionCode_H_
#define LeaderElectionCode_H_

#include "robots/blinkyBlocks/blinkyBlocksSimulator.h"
#include "robots/blinkyBlocks/blinkyBlocksWorld.h"
#include "robots/blinkyBlocks/blinkyBlocksBlockCode.h"

// Define message types for each phase
static const int TYPE_1_EXPLORE = 2001;
static const int TYPE_2_CONFIRM_CHILD = 2002;
static const int TYPE_3_REJECT_CHILD = 2003;
static const int TYPE_4_UPDATE_PLTREE = 2004;
static const int TYPE_5_RESET = 2005;
static const int TYPE_6_DISMANTLE = 2006;
static const int TYPE_7_ELECT_LEADER = 2007;
static const int TYPE_8_DISMANTLE_TREE = 2008;
static const int TYPE_9_WIN_TREE = 2009;
static const int TYPE_10_NOTCUBE = 2010;
static const int TYPE_11_CUBE_LEADER_UPDATE = 2011;
static const int TYPE_12_CUBE_CHECK = 2012;
static int NUM = 0;

using namespace BlinkyBlocks;

class leaderElectionCode : public BlinkyBlocksBlockCode {
private:
    BlinkyBlocksBlock *module = nullptr;
    P2PNetworkInterface *parentModule{};
    P2PNetworkInterface *winTreeModuleParent{};

    bool isDiscovered{};
    bool isProspectiveLeader{};
    bool isLeaf{};
    bool isExploring{};
    bool isWinTreeProcessed{};
    bool notACube = false;
    bool isDismantling = false;

    std::vector<P2PNetworkInterface *> childrenModules;
    std::vector<int> childrenSizes;

    string binaryStringId;
    int binaryIntId{};
    int weight{};
    int interfaceStatus[6]{};
    int totalConnectedInt{};
    int total{};
    int nbWaitedAnswers{};
    int myTreeTotalWeight{};
    int colorId{};

public:
    leaderElectionCode(BlinkyBlocksBlock *host);
    ~leaderElectionCode() {};

    // Primary functions for each phase
    void startup() override;

    void startCubeExploring();

    void notifyNeighborsNotACube(const std::shared_ptr<Message> &msg, P2PNetworkInterface *sender);

    void CheckIfCube(const std::shared_ptr<Message> &msg, P2PNetworkInterface *sender);

    void notifyParentYouAreACubeLeader(const std::shared_ptr<Message> &msg, P2PNetworkInterface *sender);

    void exploreNeighbors(const std::shared_ptr<Message>& msg, P2PNetworkInterface *sender);
    void confirmChild(const std::shared_ptr<Message>& msg, P2PNetworkInterface *sender);
    void electLeader(const std::shared_ptr<Message>& msg, P2PNetworkInterface *sender);
    void reset(const std::shared_ptr<Message>& msg, P2PNetworkInterface *sender);
    void dismantle(const std::shared_ptr<Message>& msg, P2PNetworkInterface *sender);
    void dismantleTree(const std::shared_ptr<Message>& msg, P2PNetworkInterface *sender);
    void winTreeUpdate(const std::shared_ptr<Message>& msg, P2PNetworkInterface *sender);

    void prospectiveLeaderTreeTotalWeightUpdate(const std::shared_ptr<Message>& msg, P2PNetworkInterface *sender);

    void rejectChild(const std::shared_ptr<Message>& msg, P2PNetworkInterface *sender);

    // Draw function for displaying information
    string onInterfaceDraw() override;

    // Configuration file parsing
    void parseUserBlockElements(TiXmlElement *config) override;

    // Code instantiation for the module
    static BlockCode *buildNewBlockCode(BuildingBlock *host) {
        return new leaderElectionCode((BlinkyBlocksBlock*)host);
    }
};

#endif /* LeaderElectionCode_H_ */

