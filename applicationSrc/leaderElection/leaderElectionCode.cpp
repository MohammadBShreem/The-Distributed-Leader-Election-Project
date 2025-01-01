#include "leaderElectionCode.hpp"

#include <thread>
#include <chrono>


leaderElectionCode::leaderElectionCode(BlinkyBlocksBlock *host) : BlinkyBlocksBlockCode(host), module(host) {
    if (!host) return;

    // Register message types with their handlers
    addMessageEventFunc2(TYPE_1_EXPLORE, [this](auto && PH1, auto && PH2) { exploreNeighbors(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2)); });
    addMessageEventFunc2(TYPE_2_CONFIRM_CHILD, [this](auto && PH1, auto && PH2) { confirmChild(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2)); });
    addMessageEventFunc2(TYPE_3_REJECT_CHILD, [this](auto && PH1, auto && PH2) { rejectChild(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2)); });

    addMessageEventFunc2(TYPE_4_UPDATE_PLTREE, [this](auto && PH1, auto && PH2) { prospectiveLeaderTreeTotalWeightUpdate(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2)); });
    addMessageEventFunc2(TYPE_5_RESET, [this](auto && PH1, auto && PH2) { reset(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2)); });
    addMessageEventFunc2(TYPE_6_DISMANTLE, [this](auto && PH1, auto && PH2) { dismantle(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2)); });
    addMessageEventFunc2(TYPE_7_ELECT_LEADER, [this](auto && PH1, auto && PH2) { electLeader(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2)); });
    addMessageEventFunc2(TYPE_8_DISMANTLE_TREE, [this](auto && PH1, auto && PH2) { dismantleTree(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2)); });
    addMessageEventFunc2(TYPE_9_WIN_TREE, [this](auto && PH1, auto && PH2) { winTreeUpdate(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2)); });
    addMessageEventFunc2(TYPE_10_NOTCUBE, [this](auto && PH1, auto && PH2) { notifyNeighborsNotACube(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2)); });
    addMessageEventFunc2(TYPE_11_CUBE_LEADER_UPDATE, [this](auto && PH1, auto && PH2) { notifyParentYouAreACubeLeader(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2)); });
    addMessageEventFunc2(TYPE_12_CUBE_CHECK, [this](auto && PH1, auto && PH2) { CheckIfCube(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2)); });
    addMessageEventFunc2(TYPE_13_REST_IN_PROCESS, [this](auto && PH1, auto && PH2) { UpdateMyParent(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2)); });

}

void leaderElectionCode::startup() {

    parentModule = nullptr;
    winTreeModuleParent = nullptr;
    isDiscovered = false;
    isExploring = false;
    isLeaf = false;
    isWinTreeProcessed = false;
    notACube = false;
    childrenModules.clear();
    childrenSizes.clear();
    binaryStringId="";
    totalConnectedInt=0;
    total=0;
    nbWaitedAnswers=0;
    myTreeTotalWeight=0;

    for(int i=0; i<module->getNbInterfaces(); i++) {
        if(module->getInterface(i)->isConnected()){
            binaryStringId += "1";
            totalConnectedInt++;
        }else{
            binaryStringId += "0";
        }
    }

    binaryIntId = weight = std::strtol(binaryStringId.c_str(), nullptr, 2);;
    console<<"Block Wight = "<<binaryIntId<<"\n";

    if(totalConnectedInt==1000 && weight<37) {
        module->setColor(RED);
        isProspectiveLeader = true;
        isDiscovered = true;
        notACube=true;
        console << "Leader " << getId() << "\n";
        colorId = NUM++;
        module->setColor(colorId);
        sendMessageToAllNeighbors("Not a cube!", new MessageOf(TYPE_10_NOTCUBE,colorId), 10, 0, 0);
        nbWaitedAnswers=sendMessageToAllNeighbors("Explore", new MessageOf(TYPE_1_EXPLORE,colorId), 100, 0, 0);
    }
    else if(totalConnectedInt==3 && weight>37) {
        //we are waiting for 500000 millisecond; to make sure if we have a block with 1 connected interface it's started & not the shape is not a Cube
        colorId = NUM++;
        nbWaitedAnswers=sendMessageToAllNeighbors("Cube Check", new MessageOf(TYPE_12_CUBE_CHECK,colorId), 500000, 0, 0);
    }
}

// Phase 1: Explore and build logical tree for each Prospective Leader;
void leaderElectionCode::exploreNeighbors(const std::shared_ptr<Message>& msg, P2PNetworkInterface *sender) {
    int colorID = *dynamic_cast<MessageOf<int>*>(msg.get())->getData();

    console << "Module " << getId() << " rcv explore message: " << sender->getConnectedBlockId() << "\n";
    if (isDiscovered) {
        sendMessage("RejectChild", new Message(TYPE_3_REJECT_CHILD), sender, 1000, 0);
    } else {
        childrenModules.clear();
        childrenSizes.clear();
        module->setColor(colorID);
        isDismantling=false;
        isRestting=false;
        isDiscovered = true;
        parentModule = sender;
        colorId = colorID;
        isExploring = false;
        nbWaitedAnswers=sendMessageToAllNeighbors("Explore", new MessageOf<int>(TYPE_1_EXPLORE,colorID), 1000, 0, 1, parentModule);
        if(nbWaitedAnswers == 0) {
            // send weight
            total+=weight;
            sendMessage("ConfirmChild", new MessageOf<int>(TYPE_2_CONFIRM_CHILD,total), sender, 1000, 0);
        }
    }
    console << "nbWaitedAnswers From Explore: " << nbWaitedAnswers << "\n";

}
void leaderElectionCode::confirmChild(const std::shared_ptr<Message>& msg, P2PNetworkInterface *sender) {
    console << "Module " << getId() << ", confirmed child ->(" << sender->getConnectedBlockId() << ")\n";
    int childWeight = *dynamic_cast<MessageOf<int>*>(msg.get())->getData();

    childrenModules.push_back(sender);
    nbWaitedAnswers--;
    total+=childWeight;
    if(nbWaitedAnswers == 0) {
        total += weight;
        if (isProspectiveLeader){ //Leader
            console <<"total: "<<total<<"\n";

            //Here we are on the PL; so now we have the Prospective Leader Total Weight of All Blocks
            myTreeTotalWeight=total;
            if (!childrenModules.empty()) {
                for (auto & childrenModule : childrenModules) {
                    sendMessage("TreeTotalWightUpdate", new MessageOf<int>(TYPE_4_UPDATE_PLTREE,myTreeTotalWeight), childrenModule, 1000, 0);
                }
            }
        }
        else if(isExploring) {
            console <<"new total: "<<total<<"\n";
            isExploring=false;
            if (parentModule)sendMessage("back the new tree total to my parent!", new MessageOf<int>(TYPE_9_WIN_TREE,total-weight), parentModule, 1000, 0);
        }
        else {
            if (parentModule)sendMessage("ConfirmChild", new MessageOf<int>(TYPE_2_CONFIRM_CHILD,total), parentModule, 1000, 0);
        }
    }
    childrenSizes.push_back(0);
}
void leaderElectionCode::rejectChild(const std::shared_ptr<Message>& msg, P2PNetworkInterface *sender) {
    nbWaitedAnswers--;
    if(nbWaitedAnswers == 0) {
        total+=weight;
        isLeaf=true;
        winTreeModuleParent=nullptr;
        if (parentModule)sendMessage("ConfirmChild", new MessageOf<int>(TYPE_2_CONFIRM_CHILD,total), parentModule, 1000, 0);
    }
    console << "Module " << getId() << ",rejected child!" << sender->getConnectedBlockId() << "\n";
    console << "nbWaitedAnswers From Reject: " << nbWaitedAnswers << "\n";
}

// Phase 2: Update each node on my Prospective Leader logical tree about the total tree total weight update
void leaderElectionCode::prospectiveLeaderTreeTotalWeightUpdate(const std::shared_ptr<Message>& msg, P2PNetworkInterface *sender) {
    int totalWeight = *dynamic_cast<MessageOf<int>*>(msg.get())->getData();
    console <<"Module "<<getId()<<", Received Tree Total Weight ("<<totalWeight << ")\n";

    myTreeTotalWeight=totalWeight;
    if (childrenModules.empty() && isLeaf) {
        sendMessageToAllNeighbors("Leader Election Started!!!!", new MessageOf(TYPE_7_ELECT_LEADER,totalWeight), 5000, 0,1, parentModule);
    }
    else if (!childrenModules.empty()){
        for (auto & childrenModule : childrenModules) {
            sendMessage("TreeTotalWightUpdate", new MessageOf<int>(TYPE_4_UPDATE_PLTREE,totalWeight), childrenModule, 1000, 0);
        }
    }
}

//Phase 3: Start Leader Election
void leaderElectionCode::electLeader(const std::shared_ptr<Message>& msg, P2PNetworkInterface *sender) {
    int received_weight = *dynamic_cast<MessageOf<int>*>(msg.get())->getData();
    console << "Module: " << getId() << " Elect Leader Started!\n";
    if (isLeaf){
        if (received_weight>myTreeTotalWeight) {
            //I Lost; notify my leader & dismantle the tree
            //notify the sender you won; start election again from his leader
            console <<"My tree Lost!\n";
            console <<"Received tree weight: " <<received_weight <<"\n";
            console <<"My tree weight: " <<myTreeTotalWeight <<"\n";
                winTreeModuleParent=sender;
                int id = static_cast<int>(getId());
                if (parentModule)sendMessage("You lost; dismantle your tree!", new MessageOf<int>(TYPE_6_DISMANTLE,id), parentModule, 100, 0);
        }
        else if (received_weight<myTreeTotalWeight) {
            //I win
            console <<"My Tree Won!\n";
            console <<"Received tree weight: " <<received_weight <<"\n";
            console <<"My tree weight: " <<myTreeTotalWeight <<"\n";
            if (parentModule)sendMessage("notifyParentIAmRestingYou", new MessageOf<int>(TYPE_13_REST_IN_PROCESS,0), parentModule, 100, 0);
            if (isLeaf && childrenModules.empty()) sendMessageToAllNeighbors("Leader Election Started!!!!", new MessageOf(TYPE_7_ELECT_LEADER,myTreeTotalWeight), 50000, 0,1, parentModule);
        }
    }
}

//Phase 4: if my TreeTotalWeight win the election; so we should start again the explore process
void leaderElectionCode::reset(const std::shared_ptr<Message>& msg, P2PNetworkInterface *sender) {
    if (!isExploring){
        console << "Module " << getId() << ", Won Message Received!\n";
        total=0;
        isExploring=true;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        nbWaitedAnswers=sendMessageToAllNeighbors("Explore", new MessageOf<int>(TYPE_1_EXPLORE,colorId), 1000, 0, 1, parentModule);
    }
}
void leaderElectionCode::winTreeUpdate(const std::shared_ptr<Message>& msg, P2PNetworkInterface *sender) {
    int newTotalUpdate = *dynamic_cast<MessageOf<int>*>(msg.get())->getData();
    if (parentModule) {
        sendMessage("back the new tree total to my parent!", new MessageOf<int>(TYPE_9_WIN_TREE,newTotalUpdate), parentModule, 1000, 0);
    }
    else if (isProspectiveLeader){
        console <<"Current Total Tree:" <<myTreeTotalWeight<<"\n";
        console <<"New Tree Total rcv: " <<newTotalUpdate<<"\n";
        myTreeTotalWeight+=newTotalUpdate;
        console <<"My Final Total Tree " <<myTreeTotalWeight <<"\n";
        isExploring=false;
        isRestting=false;
        isExploring=false;
        isDismantling=false;
        if (!childrenModules.empty()) {
            for (auto & childrenModule : childrenModules) {
                sendMessage("TreeTotalWightUpdate", new MessageOf<int>(TYPE_4_UPDATE_PLTREE,myTreeTotalWeight), childrenModule, 1000, 0);
            }
        }
    }
}

//Phase 5: if my TreeTotalWeight lost the election; so we should dismantle my tree
//to let the new explore process from the winner have all these nodes
void leaderElectionCode::dismantle(const std::shared_ptr<Message>& msg, P2PNetworkInterface *sender) {
    console << "Module " << getId() << ", dismantle!\n";
    int senderId = *dynamic_cast<MessageOf<int>*>(msg.get())->getData();

    if (parentModule == nullptr && isProspectiveLeader) {
        if (!isRestting && !isExploring && !isDismantling) {
            isDiscovered= false;
            isLeaf=false;
            isDismantling=true;
            isProspectiveLeader=false;
            total=0;
            if (!childrenModules.empty()) {
                for (auto & childrenModule : childrenModules) {
                    sendMessage("DismantleTree", new MessageOf<int>(TYPE_8_DISMANTLE_TREE,senderId), childrenModule, 100, 0);
                }
            }
        }
    }
    else {
            if(isLeaf)winTreeModuleParent=sender;
            if(parentModule) sendMessage("You lost message forward to parent!", new MessageOf<int>(TYPE_6_DISMANTLE,senderId), parentModule, 100, 0);
        }
}


void leaderElectionCode::dismantleTree(const std::shared_ptr<Message>& msg, P2PNetworkInterface *sender) {
    int senderId = *dynamic_cast<MessageOf<int>*>(msg.get())->getData();
    isDiscovered= false;
    total=0;
    //myTreeTotalWeight=0;
    console<<"senderId:____"<<senderId<<"\n";
    if(static_cast<int>(getId()) == senderId){  sendMessage("You won; start leader election again!", new MessageOf<int>(TYPE_5_RESET,colorId), winTreeModuleParent, 3000, 0); }
    else if (!childrenModules.empty()){
        for (auto & childrenModule : childrenModules) {
            sendMessage("DismantleTree", new MessageOf<int>(TYPE_8_DISMANTLE_TREE,senderId), childrenModule, 100, 0);
        }
    }
}

//Helper Functions: this for Cube Shape detection
void leaderElectionCode::notifyNeighborsNotACube(const std::shared_ptr<Message>& msg, P2PNetworkInterface *sender) {
    if (!notACube) {
        notACube=true;
        sendMessageToAllNeighbors("Not a cube!", new MessageOf(TYPE_10_NOTCUBE,0), 10, 0, 1, sender);
    }
}
void leaderElectionCode::CheckIfCube(const std::shared_ptr<Message>& msg, P2PNetworkInterface *sender) {
    int receivedColorId = *dynamic_cast<MessageOf<int>*>(msg.get())->getData();
    if (!notACube) {
        sendMessage("notifyParentYouAreACubeLeader", new MessageOf<int>(TYPE_11_CUBE_LEADER_UPDATE,receivedColorId), sender, 100000, 0);
    }
}
void leaderElectionCode::notifyParentYouAreACubeLeader(const std::shared_ptr<Message>& msg, P2PNetworkInterface *sender) {
    int receivedColorId = *dynamic_cast<MessageOf<int>*>(msg.get())->getData();
    if (!isDiscovered){
        isProspectiveLeader = true;
        console << "Leader " << getId() << "\n";
        colorId = receivedColorId;
        module->setColor(colorId);
        isDiscovered = true;
        nbWaitedAnswers=sendMessageToAllNeighbors("Explore", new MessageOf(TYPE_1_EXPLORE,colorId), 2000, 0, 0);
    }
}

void leaderElectionCode::UpdateMyParent(const std::shared_ptr<Message>& msg, P2PNetworkInterface *sender) {
    if (parentModule == nullptr && isProspectiveLeader && !isRestting) {
        //do start any rest; I'm updating you
        isRestting = true;
    }
    else {
        if (parentModule)sendMessage("notifyParentIAmRestingYou", new MessageOf<int>(TYPE_13_REST_IN_PROCESS,0), parentModule, 100, 0);
    }
}

string leaderElectionCode::onInterfaceDraw() {
    string display = "Block Id: " + std::to_string(getId()) + "\n";
    display += "my Weight (From Binary Id): " + std::to_string(weight)+ "\n";
    display += "my Tree Total Weight: " + std::to_string(total)+ "\n";
    display += "my Prospective Leader Total Weight: " + std::to_string(myTreeTotalWeight)+ "\n";
    display += "is This a Leaf Block: " + std::to_string(isLeaf)+ "\n";
    return display;
}
void leaderElectionCode::parseUserBlockElements(TiXmlElement *config) {}