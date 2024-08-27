#ifndef SERVER_STRUCTURE_H
#define SERVER_STRUCTURE_H


struct ServerNode {
    char* ip;
    float lat;
    float lon;
    struct ServerNode* next;
    struct ServerNode* prev;
};

struct ServerNode* sHead = NULL;
struct ServerNode* sTail = NULL;


struct ServerNode* createServerNode() {
    struct ServerNode* block = malloc( sizeof(struct ServerNode) );

    block->ip = NULL;
    block->next = NULL;
    block->prev = NULL;
    return block;
}

void pushServerNode(struct ServerNode* block) {
    if (sHead == NULL) {
        sHead = sTail = block;
    }
    else  {
        sTail->next = block;
        sTail->next->prev = sTail;
        sTail = sTail->next;
    }
}

void popServerNode(struct ServerNode* block) {
    if (block == NULL) return;
    else if (sHead == sTail) {
        sHead = sTail = NULL;
    }
    else if (sHead == block) {
        sHead = sHead->next;
        sHead->prev = NULL;
    }
    else if (sTail == block) {
        sTail = sTail->prev;
        sTail->next = NULL;
    }
    else {
        block->next->prev = block->prev;
        block->prev->next = block->next;
    }
    free(block); 
}

#endif