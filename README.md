# The-Distributed-Leader-Election-Project
---

authors:
  * [Mohammad SHREM](https://www.linkedin.com/in/mohammadbshreem/) <mohammad.shrem@edu.univ-fcomte.fr>
  * [Mariam AL KHALAF]() <mariam.al_khalaf@edu.univ-fcomte.fr>
  * [Idrissa MASSALY]() <idrissa.massaly@edu.univ-fcomte.fr>

supervisor:
  * [Abdallah Makhoul](https://www.femto-st.fr/en/femto-people/amakhoul) <abdallah.makhoul@univ-fcomte.fr>

project relaized in [VisibleSim](https://github.com/VisibleSim/VisibleSim)

---

##  Introduction
The algorithm is based in six phases. First, each module generates locally an integer, called weight based on the numbering of their connected interfaces. After that, spanning trees will be created using a recruitment method, then the values of the recruited modules will be calculated, the generated spanning tree will compete and the losing trees are dismantled, this step is repeated until one tree remains. And finally the leader is elected. 
</br>Computing the binary value and the weight of each block: For getting the number of interfaces we use getNbInterfaces() and generate a string by adding 1 and 0 based on the condition if connected or not respectively. After calculating the binary value, we converted the values to decimal to get the final weight value of each block. The output of this part is depicted in <i>Figure 1-a</i>.

<div align="center">
<img src="https://github.com/user-attachments/assets/f19d7c60-46ff-4a2e-b400-e142d331701d"></br>
<i>Figure 1-a: Calculation of binary number and weight value for each module</i>
</div>
</br>

The weight of each block is determined by the surfaces that have connections. Since the work was done in BlinkyBlocks, the weight of each block can take values from 0 to 32. Therefore, the potential leader can be the block whose weight is strictly equal to 1,2,4,8,16 or 32. That is, the block has only one connected neighbor (leaf).
</br>However, there are forms where each block has more than one connection. In this case, the possible leader will be selected by the lowest number of connected interfaces. That is, the block has connections to the sides that correspond to a smaller digit in a binary number:

<div align="center">
<img src="https://user-images.githubusercontent.com/74824667/110366310-65dd4c80-8046-11eb-976d-86592caf36c3.png"></br>
<i>Figure1-b: Binary number of block’s interfaces</i>
</div>
</br>

Selected potential leaders begin to recruit their children. Leaders (in our shape it is blocks with B.ID: 5 and B.ID:7) send a message to child blocks and check if this block is occupied by other leaders. If the block is busy, it stops responding. Block with ID = 10 also trying to recruit block with ID = 9, but B.ID = 9 doesn’t respond because he is already busy by B.ID = 6 and B.ID = 7 as well. Recruitment continues until the last block is occupied. 

<div align="center">
<img src="https://user-images.githubusercontent.com/74824667/110366414-8d341980-8046-11eb-918f-9b6c4c1667ab.png"></br>
<i>Figure 2: Recruitment blocks to get subtrees</i>
</div>
</br>

Finally, as many supposed leaders we have, as many subtrees we can get. To finally choose the leader, we must separately work with each subtree. Since there are two proposed leaders in our form, we got two trees, which are clearly displayed in <i>Figure 3</i>.

<div align="center">
<img src="https://user-images.githubusercontent.com/74824667/110366474-9f15bc80-8046-11eb-85d5-97ac8a478335.png"></br>
<i>Figure 3: The resulting subtrees for further comparison</i>
</div>
</br>

After the subtrees are determined, the competition of trees begins, this can be called evolution, where the strongest with the highest weight wins and becomes the root of the tree. The last leaf of the subtree reports its weight to the parent. The parent sums the received weight with its own and passes the information on. This continues until the seed (suggested leader) won't get the total sum of its entire tree. <i>Figures 4-a and 4-b</i> show a detailed scheme for obtaining the weight of subtrees for the leaders B.ID = 5 and B.ID = 7, respectively.

<div align="center">
<img src="https://user-images.githubusercontent.com/74824667/110366717-f156dd80-8046-11eb-8233-05f08f185f2e.png"></br>
<i>Figure 4-a: Getting the sum of the subtree for the leader B.ID = 5</i>
</div>
</br>

<div align="center">
<img src="https://user-images.githubusercontent.com/74824667/110366750-fae04580-8046-11eb-88a9-c8dbde5fe142.png"></br>
<i>Figure 4-b: Getting the sum of the subtree for the leader B.ID = 7</i>
</div>
</br>

When the leaders know the sum of their trees, it remains only to compare the values of all the leaders and choose the winner.


## **Components**
### **Classes and Objects**
1. **`leaderElectionCode`**:
   - The main class implementing the leader election logic.
   - Inherits from `BlinkyBlocksBlockCode`.
   - Uses a `module` object to interact with the physical or logical representation of each block.

2. **`Message` and `MessageOf`**:
   - Custom message types used for communication between modules.
   - `MessageOf<T>` is a templated message class carrying data of type `T`.

3. **`P2PNetworkInterface`**:
   - Represents a point-to-point network interface connecting modules.

---

## **Initialization**
The constructor (`leaderElectionCode`) performs:
1. **Message Handler Registration**:
   - Associates message types (e.g., `TYPE_1_EXPLORE`, `TYPE_2_CONFIRM_CHILD`) with corresponding handler functions (`exploreNeighbors`, `confirmChild`, etc.).

2. **Startup Initialization**:
   - Initializes variables like `parentModule`, `winTreeModuleParent`, and various flags (`isDiscovered`, `isLeaf`, etc.).
   - Assigns a binary ID to each module based on its network connections.

---

## **Election Phases**
### **Phase 1: Exploration and Tree Construction**
**Objective**: Build a logical tree starting from "Prospective Leaders."

**Process**:
1. **Starting Point**:
   - Nodes with only one connection (`totalConnectedInt == 1`) are designated as *Prospective Leaders*. They start the tree-building process.
   - Other nodes wait for messages (e.g., "Explore") to determine their role.

2. **`exploreNeighbors`**:
   - A node receiving an "Explore" message:
     - Joins the tree if undiscovered.
     - Sends "Explore" messages to its neighbors.
     - Updates the total weight of its subtree.

3. **`confirmChild`**:
   - Nodes confirm their children and propagate weight updates up the tree.

4. **`rejectChild`**:
   - Nodes reject redundant connections, decrementing their expected replies (`nbWaitedAnswers`).

### **Phase 2: Tree Weight Updates**
**Objective**: Calculate the total weight of each subtree rooted at a *Prospective Leader*.

**Process**:
- Each node sends its weight to its parent.
- The leader receives the total weight of its tree and informs its children using `prospectiveLeaderTreeTotalWeightUpdate`.

### **Phase 3: Leader Election**
**Objective**: Elect the leader with the heaviest tree.

**Process**:

1. **Initial Comparison**
- Every **leaf node** initiates the leader election process by comparing its tree's total weight with its neighbors.
- A node will start the process only if it is a **leaf node** and receives a leader election message.

2. **Conflict Resolution**
- To prevent multiple concurrent processes:
  - If a node loses the election, it dismantles its tree by notifying its **Prospective Leader (PL)** node.
  - The port of the winning sender is saved for future notifications.

3. **Dismantling the Tree**
- The losing node requests permission from its PL node to begin dismantling. This ensures only one process (dismantling or resetting) runs at a time.
- If the PL grants permission:
  - The node toggles on the **isDismantling flag**.
  - It starts dismantling its tree, working back to the source leaf node.
  - Upon reaching the source, the node notifies its neighbor (using the saved port) to start resetting from their side.

4. **Winning Node Reset**
- The winning node resets its tree by expanding and exploring from its **winning leaf**.
- Before resetting, it notifies its PL to ensure no other process is currently running.
- If the PL grants permission:
  - The node toggles on the **isRestting flag**.
  - going back to the request source leaf node.
  - Upon reaching the source, the node start explore and expand, ultimately assuming the role of the leader.


<div align="center">
<img src="https://github.com/user-attachments/assets/f883d382-af86-438e-b201-401d98945b66"></br>
<i>Figure: Leader Election Process</i>
</div>
</br>

### **Phase 4: Reset and Exploration**
**Objective**: Reset the system after electing a leader and restart exploration.

**Process**:
- The winning leader sends "Reset" messages to nodes.
- Nodes clear their state and prepare for new exploration.

### **Phase 5: Tree Dismantling**
**Objective**: Allow nodes to rejoin the system after their tree loses the election.

**Process**:
- Nodes receiving a "Dismantle" message detach from their tree.
- If the root dismantles, all its children are notified.

### **Special Phase: non single neighbor Structures**
**Objective**: In the event that the distributed system forms a perfect cube structure, the leader election process includes additional checks to ensure the integrity of the cube. 

**Process**:
- Nodes with one neighbour if found use specific message types (e.g., TYPE_10_NOTCUBE and TYPE_12_CUBE_CHECK) to verify whether the current arrangement satisfies cube properties. 
- if Nodes with one neighbour not found; we wait for a short time then we start this process.
- If confirmed, the cube leader is updated via TYPE_11_CUBE_LEADER_UPDATE, ensuring that The elected leader aligns with the unique geometric configuration of the system. This case demonstrates the system's adaptability to specialized topologies.

---

## **Message Types**
| **Type**                | **Purpose**                                                                 |
|-------------------------|-----------------------------------------------------------------------------|
| `TYPE_1_EXPLORE`        | Explore neighbors to build logical trees.                                  |
| `TYPE_2_CONFIRM_CHILD`  | Confirm child nodes and propagate weight updates.                          |
| `TYPE_3_REJECT_CHILD`   | Reject redundant connections.                                              |
| `TYPE_4_UPDATE_PLTREE`  | Update the total weight of the prospective leader's tree.                  |
| `TYPE_5_RESET`          | Reset the system after leader election.                                   |
| `TYPE_6_DISMANTLE`      | Dismantle a tree if it loses the election.                                 |
| `TYPE_7_ELECT_LEADER`   | Notify nodes to start leader election.                                     |
| `TYPE_8_DISMANTLE_TREE` | Propagate dismantling messages through the tree.                          |
| `TYPE_9_WIN_TREE`       | Notify the parent about the winning tree's weight.                        |
| `TYPE_10_NOTCUBE`       | Notify neighbors if the shape is not a cube.                               |
| `TYPE_11_CUBE_LEADER_UPDATE` | Inform the parent about being a cube leader.                         |
| `TYPE_12_CUBE_CHECK`    | Verify if the structure forms a cube.                                      |
| `TYPE_13_REST_IN_PROCESS` | Update parent to handle reelection scenarios.                           |

## **Key Functions**
1. **`exploreNeighbors`**:
   - Handles exploration messages and builds subtrees.
2. **`confirmChild`**:
   - Updates total subtree weights and confirms child roles.
3. **`rejectChild`**:
   - Rejects redundant connections during tree building.
4. **`prospectiveLeaderTreeTotalWeightUpdate`**:
   - Propagates the total weight of the prospective leader's tree.
5. **`electLeader`**:
   - Compares subtree weights and elects a leader.
6. **`reset`**:
   - Resets the system for further exploration.
7. **`notifyNeighborsNotACube`**:
   - Identifies non-cube structures and broadcasts information.
8. **`CheckIfCube`**:
   - Validates cube structures.
9. **`dismantle` and `dismantleTree`**:
   - Handles tree dismantling for losing trees.
---

## **Key Variables**
- **`isDiscovered`**: Indicates whether a node is part of a tree.
- **`isLeaf`**: Marks leaf nodes.
- **`nbWaitedAnswers`**: Tracks expected replies from neighbors.
- **`binaryStringId`**: Binary representation of a node's connections.
- **`weight`**: integer of a node's connections.
- **`total`**: Cumulative weight of the subtree.
- **`myTreeTotalWeight`**: Stores the total weight of the subtree for which the node is responsible.
- **`isDismantling, isResting`**: handle the status of current working process for the tree.

<div align="center">
<img src="https://github.com/user-attachments/assets/f90042a0-8c2e-4e75-88f0-a88a7cfe0ded"></br>
<i>Figure: Exploring Node Management in Distributed Systems</i>
</div>
</br>

---

## **Key Features**
1. **Fault Tolerance**:
   - If a node loses the election, its tree is dismantled, allowing it to rejoin a winning tree.

2. **Distributed Logic**:
   - Nodes operate independently, relying on message passing for coordination.

3. **Weight-Based Election**:
   - The leader is chosen based on the total weight of connected modules.
---
## **Pseudo Code**

1. **Startup Initialization**:
   -The system starts by initializing states and sending initial exploration messages.

```plaintext
startup():
    Initialize module states (parent, children, weights, flags).
    Determine binary identifier based on connected interfaces.
    If module has one interface, mark it as a prospective leader.
    Send exploration messages to neighbors.
```

---

2. **Exploration Phase**:
   - Modules explore their neighbors to discover a tree structure.

```plaintext
exploreNeighbors(msg, sender):
    If module is already discovered:
        Send rejection message to sender.
    Else:
        Mark module as discovered.
        Set sender as parent.
        Send exploration messages to all neighbors except parent.
        If no answers are awaited:
            Send confirmation message to parent with weight.
```
<div align="center">
<img src="https://github.com/user-attachments/assets/a6d2e535-2b41-4ec9-9f88-b52ee4da194c"></br>
<i>Diagram</i>
</div>
</br>
---

3. **Tree Weight Propagation**:
   - Tree weights are propagated upward to calculate the total tree weight.

```plaintext
confirmChild(msg, sender):
    Decrement the number of awaited answers.
    Add sender to children.
    Add received weight to total.
    If all answers received:
        Add self-weight to total.
        If module is a prospective leader:
            Update total weight for tree.
            Notify children with updated tree weight.
        Else:
            Send updated weight to parent.

prospectiveLeaderTreeTotalWeightUpdate(msg, sender):
    Update local tree total weight.
    If module is a leaf:
        Notify neighbors to start leader election.
    Else:
        Forward updated weight to children.
```

---

4. **Leader Election**:
   - Modules compare weights to elect a leader. Trees are dismantled if lost.

```plaintext
electLeader(msg, sender):
    Compare received weight with local tree weight.
    If received weight is greater:
        Mark the tree as lost.
	save sender port as WinPort.
        start leader election again.
    Else:
        Mark the tree as the winner.
        Notify sender to dismantle its tree.
```

---

5. **Reset and Recovery**:
   -Modules reset their state to recover from leader election failures or conflicts.

```plaintext
reset(msg, sender):
    Reinitialize state variables.
    Send exploration messages to neighbors.

winTreeUpdate(msg, sender):
    Update total weight of tree.
    If module is the root:
        Notify children with updated tree weight.
    Else:
        Forward updated weight to parent.
```

---

6. **Dismantling Trees**:
   -When a tree loses in the election process, it is dismantled to allow recovery and reelection.

```plaintext
dismantle(msg, sender):
    If module is a prospective leader:
        Reset state and notify children to dismantle their trees.
    Else:
        Forward dismantle message to parent.

dismantleTree(msg, sender):
    Reset module state.
    If module is a leaf && has a WinPort:
        Notify parent about dismantling completion.
```

<div align="center">
<img src="https://github.com/user-attachments/assets/bc347406-c36a-4ffa-987e-f06db61fe285"></br>
<i>Diagram</i>
</div>
</br>

---

## 	Experimental part
In the next paragraph, we provide the results of our simulation, as well as the execution time of the algorithm and the number of messages sent.
<div align="center">
<img src="https://github.com/user-attachments/assets/dd08ab01-eae8-435a-b676-8b23b63065aa"></br>
<i>Figure 5: Final weight of subtrees & it's new total on a Leaf</i>
</div>
</br>
1. **Chain Model**:
The process result is shown below; we started with chain model of 199 block as shown in <i>Figure 6-a</i> and experimented in <i>Figure 6-b</i>.
</br>
<div align="center">
<img src="https://user-images.githubusercontent.com/74824667/110368891-eb163080-8049-11eb-8ee7-90110caeebbf.png"></br>
<i>Figure 6-a: diagram for chain model</i>
</div>
</br>
<div align="center">
<img src="https://github.com/user-attachments/assets/9ab3591e-4e1e-42a9-bc34-524eacfbc8c4"></br>
<i>Figure 6-b: simulation of obvious model</i>
</div>
</br>

2. **Cube Model**:

<div align="center">
<img src="https://github.com/user-attachments/assets/1786c6d1-ca82-4adf-b78c-d4393266d7d7"></br>
<i>Figure 7: simulation of cube model</i>
</div>


4. **Random Model**:

<div align="center">
<img src="https://github.com/user-attachments/assets/0a88cfe2-c613-4487-a2bb-08f9774e3632"></br>
<i>Figure8: simulation of random model</i>
</div>

5. **Videos**:


https://github.com/user-attachments/assets/21365bcb-17a9-4f2f-8c4f-870ab2c0a12d
<div align="center">
<i>Figure 9: simulation video</i>
</div>

6. **Ploting**:

<div align="center">
<img src="https://github.com/user-attachments/assets/147f7a12-1700-45ef-841b-407bebbf1aa9"></br>
<i>Figure 10-a: Linear Plot: Number Of Robot</i>
</div>

<div align="center">
<img src="https://github.com/user-attachments/assets/ba70b8ac-3c72-4457-be95-d5db94115c42"></br>
<i>Figure 10-b: Linear Plot: Elapsed Time</i>
</div>

<div align="center">
<img src="https://github.com/user-attachments/assets/4f526058-91b5-4257-bc5b-e14291358067"></br>
<i>Figure 10-c: Number of Events Processed</i>
</div>

<div align="center">
<img src="https://github.com/user-attachments/assets/c010d277-cddc-4b7e-bd54-2def8523f258"></br>
<i>Figure 10-d: Number of Messages Processed</i>
</div>

<div align="center">
<img src="https://github.com/user-attachments/assets/a68a29fb-f557-45a7-8ed1-74e634ee9e20"></br>
<i>Figure 10-e: Combined Plot of Metrics Across Scenarios</i>
</div>

##  Conclusions
This algorithm is needed not only to play the evolution of digital trees, but also has a very useful and important application. in decentralized systems where the task is to select a leader, this algorithm can be used on an equal footing with the ABC-Center, k-BFS SumSweep, etc.
Note: We faced a time glitch; which not the same based on the machine or laptop


