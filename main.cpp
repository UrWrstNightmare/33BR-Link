#include <bits/stdc++.h>

#define DEFAULT 0
#define PRIORITY 1
#define SUPER_PRIORITY 2

#define BUFFER_MAX_SIZE 100
#define PRIORITY_BUFFER_MAX_SIZE 20
#define NETIN_BUFFER_MAX_SIZE 140

#define PACKET_STR_SIZE 10

#define PROGRAM_MAX_NUMBER 10
#define UUID_MAX_NUMBER 20

#define MAX_PACKETS_SEND 1

using namespace std;

struct sysdata{
    //buffer read & write priority
    short int readPos = BUFFER_MAX_SIZE-1; //for sending
    short int readPosPriority = PRIORITY_BUFFER_MAX_SIZE-1;
    short int readPosSuperPriority = PRIORITY_BUFFER_MAX_SIZE-1;
    //for buffer write
    short int writePos = BUFFER_MAX_SIZE-1;
    short int writePosPriority = PRIORITY_BUFFER_MAX_SIZE-1;
    short int writePosSuperPriority = PRIORITY_BUFFER_MAX_SIZE-1;
    
    //UUID
    short int lastUUID = 0;
    short int activeUUID = 0;
    
};

struct packet{
    short int UUID; //Defines the UUID of the packet, defined by the system
    short int STATE = -5; //Defines the STATE of the packet, if positive means the # of the packet in sequence
    /*
     * positive: 0 1 2 3 4 5... --> consecutive packet order
     * negative:
     * -1: ERR
     * -2: SYS
     * -3: ACK
     * -4: END
     * -5: EMPTY
     */
    short int programID; //Defines the PROGRAMID of the packet, to distinguish between sender and reciever programs.
    string message;
    short int NEXTMEM = -1; //-1 is UNLINKED
};

struct packet tcp_netout[BUFFER_MAX_SIZE]; //default
struct packet tcp_netout_priority[PRIORITY_BUFFER_MAX_SIZE]; //priority
struct packet ble_netout[PRIORITY_BUFFER_MAX_SIZE]; //super_priority

struct sysdata data;

struct packet tcp_netin[NETIN_BUFFER_MAX_SIZE];
int storeIndex=0;
int TcpinStartMem[PROGRAM_MAX_NUMBER];
int TcpinEndMem[PROGRAM_MAX_NUMBER];
int activeUUID[UUID_MAX_NUMBER];

void print_tcp_netin(){
    cout << "[DEBUG] TCP_NETIN buffer state:\n --> UUID: ";
    for(int i = 0 ; i< NETIN_BUFFER_MAX_SIZE; i++){
        cout << tcp_netin[i].UUID << " ";
    }
    cout << "\n --> STATE: ";
    for(int i = 0 ; i< NETIN_BUFFER_MAX_SIZE; i++){
        cout << tcp_netin[i].STATE << " ";
    }
    cout << "\n --> NEXTMEM: ";
    for(int i = 0 ; i< NETIN_BUFFER_MAX_SIZE; i++){
        cout << tcp_netin[i].NEXTMEM << " ";
    }
    cout << "\n----------------------";
}

void print_startmem(){
    cout << "[DEBUG] STARTMEM buffer state:\n";
    for(int i = 0 ; i < NETIN_BUFFER_MAX_SIZE; i++){
        cout << TcpinStartMem[i] << " ";
    }
    cout << "\n----------------------";
}

void print_endmem(){
    cout << "[DEBUG] ENDMEM buffer state:\n";
    for(int i = 0; i < NETIN_BUFFER_MAX_SIZE; i++){
        cout << TcpinEndMem[i] << " ";
    }
    cout << "\n----------------------";
}

int getNetinIndex(int index) {
    if (index<0) {
        return index+NETIN_BUFFER_MAX_SIZE;
    }
    else if(index<NETIN_BUFFER_MAX_SIZE) {
        return index;
    }
    else return index-NETIN_BUFFER_MAX_SIZE;
}


void circularIndex(int d) {
    storeIndex=getNetinIndex(storeIndex+d);
}


short int tcp_reciever(packet p) {
    cout << "[TCP] Sending packet UUID: " << p.UUID << " ProgNo: " << p.programID << " STATE: " << p.STATE
    << "message: " << p.message << endl;
    
    
    //Determine Buffer is Empty or not
    if(tcp_netin[storeIndex].STATE!=-5) {
        while (storeIndex++ >= NETIN_BUFFER_MAX_SIZE) {
            if (tcp_netin[storeIndex].STATE==-5) break;
        }
        if (storeIndex==NETIN_BUFFER_MAX_SIZE) {
            printf("[TcpRecv] [!] BUFFER EXCEED! Cannot receive anymore (-1)\n");
            return -1; //-1: Buffer Exceed
        }
    }
    
    //React by State
    else if (p.STATE==-1) {
        printf("[TcpRecv] [!] Received ERROR Data (-1) (U: %d, P: %d)\n", tcp_netin[storeIndex].UUID, tcp_netin[storeIndex].programID);
    }
    
    else if (p.STATE==-2) {
        //System
    }
    
    else if (p.STATE==-3) {
        //Acknowledge
    }
    /*
     else if (p.STATE==-4) {
     //End
     }
     */
    
    else if (p.STATE==-5) {
        //Empty
        circularIndex(-1);
        printf("[TcpRecv] [!] Received EMPTY Data (-5) (U: %d, P: %d)\n", tcp_netin[storeIndex].UUID, tcp_netin[storeIndex].programID);
        return 0;
    }
    
    tcp_netin[storeIndex]=p;
    printf("[TcpRecv] Received Data Successfully (U: %d, P: %d)\n", tcp_netin[storeIndex].UUID, tcp_netin[storeIndex].programID);
    printf("> \"%s\"\n", tcp_netin[storeIndex].message.c_str());
    for (int i=0; i<NETIN_BUFFER_MAX_SIZE; i++) {
        printf("%d ", tcp_netin[i].UUID);
    }
    printf("\n");
    
    //Check primary address of larger UUID
    if(tcp_netin[getNetinIndex(storeIndex-1)].STATE!=-4&&tcp_netin[getNetinIndex(storeIndex-1)].UUID<tcp_netin[storeIndex].UUID) activeUUID[tcp_netin[storeIndex].UUID]=storeIndex;
    else if (tcp_netin[storeIndex].STATE!=-4) activeUUID[tcp_netin[storeIndex].UUID]=-1;
    
    //Buffer Address Record
    if (tcp_netin[getNetinIndex(storeIndex-1)].programID!=tcp_netin[storeIndex].programID) {
        TcpinStartMem[tcp_netin[storeIndex].programID]=storeIndex;
        TcpinEndMem[tcp_netin[storeIndex].programID]=storeIndex;
        tcp_netin[getNetinIndex(TcpinEndMem[tcp_netin[storeIndex].programID])].NEXTMEM=storeIndex;
    }
    
    else {
        if (activeUUID[tcp_netin[storeIndex].UUID]!=-1) {
            tcp_netin[TcpinEndMem[tcp_netin[storeIndex].programID]].NEXTMEM=storeIndex;
        }
        else {
            for (int i=0; i<storeIndex; i++) {
                if (tcp_netin[i].UUID>tcp_netin[storeIndex].UUID)
                    tcp_netin[TcpinEndMem[tcp_netin[storeIndex].UUID]].NEXTMEM=i;
                }
            /*
            for (int i=storeIndex+1; i<BUFFER_MAX_SIZE; i++) {
                if (tcp_netin[i].UUID>tcp_netin[storeIndex].UUID)
            }
             */ //Should cope with this case (cause of cirular structure of buffer)
        }
        
        //Replace End Memory of UUID
        TcpinEndMem[tcp_netin[storeIndex].programID]=storeIndex;
    }
    
    //Makes Buffer Circular Structure
    storeIndex=getNetinIndex(storeIndex++);
    
    
    return 0;
    
} //function to recieve functions
void ble_reciever(packet p){
    cout << "[BLE] Sending packet UUID: " << p.UUID << " ProgNo: " << p.programID << " STATE: " << p.STATE << "message: " << p.message << endl;
};

short int getPartCount(short int len){
    return len/PACKET_STR_SIZE + 1;
}

short int getUUID(){
    if(data.lastUUID == 32767)
        data.lastUUID = 0;
    else
        data.lastUUID++;
    
    data.activeUUID++;
    
    if(data.activeUUID > UUID_MAX_NUMBER){
        data.activeUUID--;
        return -1;
    }
    
    
    return data.lastUUID;
}

short int getNextFreeBuffer(){
    if(data.writePos == BUFFER_MAX_SIZE-1)
        data.writePos = 0;
    else
        data.writePos++;
    
    if(tcp_netout[data.writePos].STATE != -5)
        return -1; //BUFFER FULL ERROR
    
    return data.writePos;
}

short int getNextFreeBufferPriority(){
    if(data.writePosPriority == PRIORITY_BUFFER_MAX_SIZE-1)
        data.writePosPriority = 0;
    else
        data.writePosPriority++;
    
    if(tcp_netout_priority[data.writePosPriority].STATE != -5)
        return -1; //BUFFER FULL ERROR
    
    return data.writePosPriority;
}

short int getNextFreeBufferSuperPriority(){
    if(data.writePosSuperPriority == PRIORITY_BUFFER_MAX_SIZE-1)
        data.writePosSuperPriority = 0;
    else
        data.writePosSuperPriority++;
    
    if(ble_netout[data.writePosSuperPriority].STATE != -5)
        return -1; //BUFFER FULL ERROR
    
    return data.writePosSuperPriority;
}

void freeBuffer(short int pos){
    
    short int lastpos;
    short int cnt=0; //unnecessary feature...
    if(pos == -1)
        return;
    data.writePos = pos -1 ;
    if(data.writePos < 0)
        data.writePos = BUFFER_MAX_SIZE-1;
    do{
        cnt++;
        tcp_netout[pos].STATE = -5; //empty
        lastpos = pos;
        pos = tcp_netout[pos].NEXTMEM;
        tcp_netout[lastpos].NEXTMEM = -1;
    }while(pos != -1);
    cout << "[FreeBuffer] Freed " << cnt << " memory from DEFAULT_BUFFER!"<< endl;
}

void freeBufferPriority(short int pos){
    
    short int lastpos;
    short int cnt=0; //unnecessary feature...
    if(pos == -1)
        return;
    data.writePosPriority = pos-1;
    if(data.writePosPriority < 0)
        data.writePosPriority = PRIORITY_BUFFER_MAX_SIZE-1;
    do{
        cnt++;
        tcp_netout_priority[pos].STATE = -5; //empty
        lastpos = pos;
        pos = tcp_netout_priority[pos].NEXTMEM;
        tcp_netout_priority[lastpos].NEXTMEM = -1;
    }while(pos != -1);
    cout << "[FreeBuffer] Freed " << cnt << " memory from PRIORITY_BUFFER!"<< endl;
}

void freeBufferSuperPriority(short int pos){
    short int lastpos;
    short int cnt=0; //unnecessary feature...
    if(pos == -1)
        return;
    data.writePosSuperPriority = pos-1;
    if(data.writePosSuperPriority < 0)
        data.writePosSuperPriority = PRIORITY_BUFFER_MAX_SIZE-1;
    do{
        cnt++;
        ble_netout[pos].STATE = -5; //empty
        lastpos = pos;
        pos = ble_netout[pos].NEXTMEM;
        ble_netout[lastpos].NEXTMEM = -1;
    }while(pos != -1);
    cout << "[FreeBuffer] Freed " << cnt << " memory from BLE_BUFFER!"<< endl;
}

short int send(string msg, short int progNo, short int priority){
    struct packet temp;
    
    short int len = (short int)msg.length();
    short int count = getPartCount(len);
    
    temp.UUID = getUUID(); //generates an unique UUID for the message;
    if(temp.UUID == -1){
        cout << "[SEND ERROR] TOO MANY ACTIVE UUID..." << endl;
        return -2; //TO MANY UUID
    }
    
    temp.programID = progNo;
    if(temp.programID < 0 || temp.programID > PROGRAM_MAX_NUMBER){
        cout << "[SEND ERROR] INVALID PROGRAM ID PASSED..." << endl;
        return -3; //INVALID PROGRAM ID
    }
    short int lastMEMloc = -1;
    short int pos;
    
    for(short int i = 0; i < count; i++){
        
        short int msgStartPos = 0;
        
        if(i == count-1){
            temp.STATE = -4; //END
            temp.message = msg.substr(PACKET_STR_SIZE*i, (len-1)%10+1);
        }
        else{
            temp.STATE = i;
            temp.message = msg.substr(PACKET_STR_SIZE*i, PACKET_STR_SIZE);
        }
        
        switch(priority){
            case 0:
                pos = getNextFreeBuffer();
                if(i == 0)
                    msgStartPos = pos;
                if(pos < 0){
                    cout<<"[SEND ERROR] NO MORE BUFFER AVAILABLE @ DEFAULT_BUFFER!" << endl;
                    freeBuffer(msgStartPos);
                    return -1;
                }
                tcp_netout[pos] = temp; //put item in memory
                if(lastMEMloc != -1)
                    tcp_netout[lastMEMloc].NEXTMEM = pos; //linked list creation!
                lastMEMloc = pos;
                break;
            case 1:
                pos = getNextFreeBufferPriority();
                if(i == 0)
                    msgStartPos = pos;
                if(pos < 0){
                    cout<<"[SEND ERROR] NO MORE BUFFER AVAILABLE @ PRIORITY_BUFFER!" << endl;
                    freeBufferPriority(msgStartPos);
                    return -1;
                }
                tcp_netout_priority[pos] = temp; //put item in memory
                if(lastMEMloc != -1)
                    tcp_netout_priority[lastMEMloc].NEXTMEM = pos; //linked list creation!
                lastMEMloc = pos;
                break;
            case 2:
                pos = getNextFreeBufferSuperPriority();
                if(i == 0)
                    msgStartPos = pos;
                if(pos < 0){
                    cout<<"[SEND ERROR] NO MORE BUFFER AVAILABLE @ BLE_BUFFER!" << endl;
                    freeBufferSuperPriority(msgStartPos);
                    return -1;
                }
                ble_netout[pos] = temp; //put item in memory
                if(lastMEMloc != -1)
                    ble_netout[lastMEMloc].NEXTMEM = pos; //linked list creation!
                lastMEMloc = pos;
                break;
            default:
                cout << "[SEND ERROR] Invalid PROGNO Argument passed..." << endl;
                return -4; //INVALID ARGS
        }
    }
    return -20;
}

short int readBLEBuffer(){
    short int prev = data.readPosSuperPriority;
    if(data.readPosSuperPriority == PRIORITY_BUFFER_MAX_SIZE-1)
        data.readPosSuperPriority = 0;
    else
        data.readPosSuperPriority++;
    if((prev == data.writePosSuperPriority&&ble_netout[prev].STATE ==-5) || ble_netout[data.readPosSuperPriority].STATE == -5) {
        cout << "[READ BLE] No More to read in BLE buffer..." << endl;
        data.readPosSuperPriority = prev;
        return -1;
    } else
        return data.readPosSuperPriority;
}

short int readPriorityBuffer(){
    short int prev = data.readPosPriority;
    if(data.readPosPriority == PRIORITY_BUFFER_MAX_SIZE-1)
        data.readPosPriority = 0;
    else
        data.readPosPriority++;
    if( (prev == data.writePosPriority && tcp_netout_priority[prev].STATE==-5) || tcp_netout_priority[data.readPosPriority].STATE == -5) {
        cout << "[READ PRIORITY] No More to read in PRIORITY buffer..." << endl;
        data.readPosPriority = prev;
        return -1;
    } else
        return data.readPosPriority;
}

short int readBuffer(){
    short int prev = data.readPos;
    if(data.readPos == BUFFER_MAX_SIZE-1)
        data.readPos = 0;
    else
        data.readPos++;
    
    cout << "[READ TCP] Reading buffer " << data.readPos << endl;
    if((prev == data.writePos && tcp_netout[prev].STATE == -5) || tcp_netout[data.readPos].STATE == -5) {
        cout << "[READ TCP] No More to read in TCP buffer..." << endl;
        data.readPos = prev;
        return -1;
    } else
        return data.readPos;
}

void getTCPbufferState(){ //for DEBUG
    cout << "[TCP BUFFER] Current Buffer image:" << endl;
    short int cnt = 0;
    for(short int i = 0; i < BUFFER_MAX_SIZE; i++){
        if(tcp_netout[i].STATE != -5){
            cnt++;
            cout << tcp_netout[i].UUID << " ";
        } else
            cout << "0 ";
    }
    cout << "\n";
    cout << "[TCP BUFFER] " << cnt << "/" << BUFFER_MAX_SIZE << " (" << (double)cnt/BUFFER_MAX_SIZE*100 << "% Full)" << endl;
}
void getPrioritybufferState(){ //for DEBUG
    cout << "[PRIORITY BUFFER] Current Buffer image:" << endl;
    short int cnt = 0;
    for(short int i = 0; i < PRIORITY_BUFFER_MAX_SIZE; i++){
        if(tcp_netout_priority[i].STATE != -5){
            cnt++;
            cout << tcp_netout_priority[i].UUID << " ";
        } else
            cout << "0 ";
    }
    cout << "\n";
    cout << "[PRIORITY BUFFER] " << cnt << "/" << PRIORITY_BUFFER_MAX_SIZE << " (" << (double)cnt/PRIORITY_BUFFER_MAX_SIZE*100 << "% Full)" << endl;
}

void getBLEbufferState(){ //for DEBUG
    cout << "[BLE BUFFER] Current Buffer image:" << endl;
    short int cnt = 0;
    for(short int i = 0; i < PRIORITY_BUFFER_MAX_SIZE; i++){
        if(ble_netout[i].STATE != -5){
            cnt++;
            cout << ble_netout[i].UUID << " ";
        } else
            cout << "0 ";
    }
    cout << "[BLE BUFFER] Current Buffer STATE:" << endl;
    cnt = 0;
    for(short int i = 0; i < PRIORITY_BUFFER_MAX_SIZE; i++){
        if(ble_netout[i].STATE != -5){
            cnt++;
            cout << ble_netout[i].STATE << " ";
        } else
            cout << "0 ";
    }
    cout << "\n";
    cout << "[BLE BUFFER] " << cnt << "/" << PRIORITY_BUFFER_MAX_SIZE << " (" << (double)cnt/PRIORITY_BUFFER_MAX_SIZE*100 << "% Full)" << endl;
}



short int proc(){
    short int pos;
    for(short int i = 0; i < MAX_PACKETS_SEND; i++){
        pos = readBLEBuffer();
        if(pos == -1)
            break;
        ble_reciever(ble_netout[pos]);
        ble_netout[pos].UUID = 0; //debug
        ble_netout[pos].STATE = -5; //EMPTY
    }
    short int i;
    for(i = 0; i < MAX_PACKETS_SEND; i++){
        pos = readPriorityBuffer();
        if(pos == - 1)
            break;
        tcp_reciever(tcp_netout_priority[pos]);
        tcp_netout_priority[pos].UUID = 0; //debug
        tcp_netout_priority[pos].STATE = -5;
    }
    for( ;i<MAX_PACKETS_SEND; i++){
        pos =  readBuffer();
        if(pos == -1)
            break;
        tcp_reciever(tcp_netout[pos]);
        tcp_netout[pos].UUID = 0; //debug
        tcp_netout[pos].STATE = -5;
    }
    return -20;
}

int main(void){
    bool terminate = false;
    while(!terminate){
        cout << "Processing Packets..."<<endl;
        proc();
        char command[10000];
        string tmp;
        while(tmp != "proc"){
            scanf("%s", command);
            tmp = command;
            if(tmp =="sendTCP"){
                cout << "[SendTCP] Enter Message..." << endl;
                fflush(stdin);
                gets(command);
                int progNo;
                cout << "[SendTCP] Enter progID" << endl;
                scanf("%d", &progNo);
                string msg = command;
                send(msg, (short int) progNo, DEFAULT);
            }
            else if (tmp == "sendPRIORITY"){
                cout << "[SendPRIORITY] Enter Message..." << endl;
                fflush(stdin);
                gets(command);
                int progNo;
                cout << "[SendPRIORITY] Enter progID" << endl;
                scanf("%d", &progNo);
                string msg = command;
                send(msg, (short int) progNo, PRIORITY);
            }
            else if (tmp == "sendBLE"){
                cout <<"[SendBLE] Enter Message..." << endl;
                fflush(stdin);
                gets(command);
                int progNo;
                cout << "[SendBLE] Enter progID" << endl;
                scanf("%d", &progNo);
                string msg = command;
                send(msg, (short int) progNo, SUPER_PRIORITY);
            }
            else if(tmp == "stateTCP"){
                getTCPbufferState();
            }
            else if(tmp == "statePRIORITY"){
                getPrioritybufferState();
            }
            else if(tmp == "stateBLE"){
                getBLEbufferState();
            }
            else if(tmp == "ack"){
                if(data.activeUUID > 0){
                    cout << "[SYSTEM] ACK message FORCED! Active UUIDs': " << data.activeUUID <<  endl;
                    data.activeUUID--;
                }else{
                    cout << "[SYSTEM] There are no active UUIDS... Command execution interrupted" << endl;
                }
                
            }
            else if(tmp == "netin"){
                print_tcp_netin();
            }
            else if(tmp == "startmem"){
                print_startmem();
            }
            else if(tmp == "endmem"){
                print_endmem();
            }
            else{
                cout << "Unknown Command..." << endl;
            }
        }
    }
}
