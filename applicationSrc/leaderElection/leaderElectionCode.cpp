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
            interfaceStatus[i]=1;
        }else{
            binaryStringId += "0";
            interfaceStatus[i]=0;
        }
    }

    binaryIntId = weight = std::strtol(binaryStringId.c_str(), nullptr, 2);;
    console<<"Block Wight = "<<binaryIntId<<"\n";

    if(totalConnectedInt==1) {
        module->setColor(RED);
        isProspectiveLeader = true;
        isDiscovered = true;
        notACube=true;
        console << "Leader " << getId() << "\n";
        colorId = NUM++;
        module->setColor(colorId);
        sendMessageToAllNeighbors("Not a cube!", new MessageOf(TYPE_10_NOTCUBE,colorId), 10, 0, 0);
        nbWaitedAnswers=sendMessageToAllNeighbors("Explore", new MessageOf(TYPE_1_EXPLORE,colorId), 1000, 0, 0);
    }
    else if(totalConnectedInt==3 && weight>37) {
        //we are waiting for 20000 millisecond; to make sure if we have a block with 1 connected interface it's started & not the shape is not a Cube
        colorId = NUM++;
        nbWaitedAnswers=sendMessageToAllNeighbors("Cube Check", new MessageOf(TYPE_12_CUBE_CHECK,colorId), 20000, 0, 0);

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
        isDiscovered = true;
        parentModule = sender;
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
            for (auto & childrenModule : childrenModules) {
                sendMessage("TreeTotalWightUpdate", new MessageOf<int>(TYPE_4_UPDATE_PLTREE,myTreeTotalWeight), childrenModule, 1000, 0);
            }
        }
        else if(isExploring) {
            console <<"new total: "<<total<<"\n";
            isExploring=false;
            sendMessage("back the new tree total to my parent!", new MessageOf<int>(TYPE_9_WIN_TREE,total-weight), parentModule, 1000, 0);
        }
        else {
            sendMessage("ConfirmChild", new MessageOf<int>(TYPE_2_CONFIRM_CHILD,total), parentModule, 1000, 0);
        }
    }
    childrenSizes.push_back(0);
}
void leaderElectionCode::rejectChild(const std::shared_ptr<Message>& msg, P2PNetworkInterface *sender) {
    nbWaitedAnswers--;
    if(nbWaitedAnswers == 0) {
        total+=weight;
        if (childrenModules.empty())isLeaf=true;
        sendMessage("ConfirmChild", new MessageOf<int>(TYPE_2_CONFIRM_CHILD,total), parentModule, 1000, 0);
    }
    console << "Module " << getId() << ",rejected child!" << sender->getConnectedBlockId() << "\n";
    console << "nbWaitedAnswers From Reject: " << nbWaitedAnswers << "\n";
}

// Phase 2: Update each node on my Prospective Leader logical tree about the total tree total weight update
void leaderElectionCode::prospectiveLeaderTreeTotalWeightUpdate(const std::shared_ptr<Message>& msg, P2PNetworkInterface *sender) {
    int totalWeight = *dynamic_cast<MessageOf<int>*>(msg.get())->getData();
    console <<"Module "<<getId()<<", Received Tree Total Weight ("<<totalWeight << ")\n";

    myTreeTotalWeight=totalWeight;
    if (childrenModules.empty() && isLeaf && nbWaitedAnswers == 0 && myTreeTotalWeight>0) {
        sendMessageToAllNeighbors("Leader Election Started!", new MessageOf(TYPE_7_ELECT_LEADER,totalWeight), 80000, 0,1, sender);
    }
    else {
        for (auto & childrenModule : childrenModules) {
            sendMessage("TreeTotalWightUpdate", new MessageOf<int>(TYPE_4_UPDATE_PLTREE,totalWeight), childrenModule, 1000, 0);
        }
    }
}

//Phase 3: Start Leader Election
void leaderElectionCode::electLeader(const std::shared_ptr<Message>& msg, P2PNetworkInterface *sender) {
    int received_weight = *dynamic_cast<MessageOf<int>*>(msg.get())->getData();
    console << "Module: " << getId() << " Elect Leader Started!\n";

    if (received_weight>myTreeTotalWeight && myTreeTotalWeight>0 && isDiscovered && isLeaf) {
        //I Lost; notify my leader & dismantle the tree
        //notify the sender you won; start election again from his leader
        console <<"My tree Lost!\n";
        console <<"Received tree weight: " <<received_weight <<"\n";
        console <<"My tree weight: " <<myTreeTotalWeight <<"\n";
        winTreeModuleParent=sender;
    }
    else if (received_weight<myTreeTotalWeight && myTreeTotalWeight>0 && isDiscovered && isLeaf) {
        //I win
        console <<"My Tree Won!\n";
        console <<"Received tree weight: " <<received_weight <<"\n";
        console <<"My tree weight: " <<myTreeTotalWeight <<"\n";
        sendMessage("You lost; dismantle your tree!", new MessageOf<int>(TYPE_6_DISMANTLE,0), sender, 1000, 0);
    }
}

//Phase 4: if my TreeTotalWeight win the election; so we should start again the explore process
void leaderElectionCode::reset(const std::shared_ptr<Message>& msg, P2PNetworkInterface *sender) {
    console << "Module " << getId() << ", Won Message Received!\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    total=0;
    isExploring=true;
    nbWaitedAnswers=sendMessageToAllNeighbors("Explore", new MessageOf<int>(TYPE_1_EXPLORE,colorId), 1000, 0, 1, parentModule);
}
void leaderElectionCode::winTreeUpdate(const std::shared_ptr<Message>& msg, P2PNetworkInterface *sender) {
    int newTotalUpdate = *dynamic_cast<MessageOf<int>*>(msg.get())->getData();
    if (parentModule) {
        sendMessage("back the new tree total to my parent!", new MessageOf<int>(TYPE_9_WIN_TREE,newTotalUpdate), parentModule, 1000, 0);
    }
    else {
        console <<"Current Total Tree:" <<myTreeTotalWeight<<"\n";
        console <<"New Tree Total rcv: " <<newTotalUpdate<<"\n";
        myTreeTotalWeight+=newTotalUpdate;
        console <<"My Final Total Tree " <<myTreeTotalWeight <<"\n";
        for (auto & childrenModule : childrenModules) {
            sendMessage("TreeTotalWightUpdate", new MessageOf<int>(TYPE_4_UPDATE_PLTREE,myTreeTotalWeight), childrenModule, 1000, 0);
        }
    }
}

//Phase 5: if my TreeTotalWeight lost the election; so we should dismantle my tree
//to let the new explore process from the winner have all these nodes
void leaderElectionCode::dismantle(const std::shared_ptr<Message>& msg, P2PNetworkInterface *sender) {
    console << "Module " << getId() << ", dismantle!\n";

    if (parentModule == nullptr && isProspectiveLeader && !isDismantling) {
        isDiscovered= false;
        isLeaf=false;
        isDismantling=true;
        isProspectiveLeader= false;
        total=0;
        myTreeTotalWeight=0;
        for (auto & childrenModule : childrenModules) {
            sendMessage("DismantleTree", new MessageOf<int>(TYPE_8_DISMANTLE_TREE,0), childrenModule, 1000, 0);
        }
    }
    else {
        //if(isLeaf)winTreeModuleParent=sender;
        if(parentModule)sendMessage("You lost message forward to parent!", new MessageOf<int>(TYPE_6_DISMANTLE,0), parentModule, 1000, 0);
    }
}
void leaderElectionCode::dismantleTree(const std::shared_ptr<Message>& msg, P2PNetworkInterface *sender) {
    isDiscovered= false;
    total=0;
    myTreeTotalWeight=0;
    if(isLeaf && winTreeModuleParent != nullptr && childrenModules.empty())sendMessage("You won; start leader election again!", new MessageOf<int>(TYPE_5_RESET,0), winTreeModuleParent, 3000, 0);
    else{
        for (auto & childrenModule : childrenModules) {
            sendMessage("DismantleTree", new MessageOf<int>(TYPE_8_DISMANTLE_TREE,0), childrenModule, 1000, 0);
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
        sendMessage("notifyParentYouAreACubeLeader", new MessageOf<int>(TYPE_11_CUBE_LEADER_UPDATE,receivedColorId), sender, 1000, 0);
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

string leaderElectionCode::onInterfaceDraw() {
    string display = "Block Id: " + std::to_string(getId()) + "\n";
    display += "my Weight (From Binary Id): " + std::to_string(weight)+ "\n";
    display += "my Tree Total Weight: " + std::to_string(total)+ "\n";
    display += "my Prospective Leader Total Weight: " + std::to_string(myTreeTotalWeight)+ "\n";
    display += "is This a Leaf Block: " + std::to_string(isLeaf)+ "\n";
    return display;
}
void leaderElectionCode::parseUserBlockElements(TiXmlElement *config) {}