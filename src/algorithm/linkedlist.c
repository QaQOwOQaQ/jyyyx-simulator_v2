#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <headers/algorithm.h>


class cycleList {
private:
    typedef struct Node_t {
        string val;
        Node_t *next;
        Node_t() : val(""), next(nullptr) {}
        Node_t(string _val) : val(_val), next(nullptr) {}
        Node_t(string _val, Node_t *_next) : val(_val), next(_next) {}
    } node;
    node *head;
    
public:
    cycleList() {
        head = new node;
        head->next = head;
    }
    
    // add val at link head
    void add(const string &val) {   
        node *newNode = new node(val);
        newNode->next = head->next;
        head->next = newNode;
    }
    
    // return true if delete successful
    // return false if this val dont't exist
    bool erase(const string &val) {
        node *cur = head;
        while(cur->next != head) {
            if(cur->next->val == val) {
                cur->next = cur->next->next;
                return true;
            }
            cur = cur->next;
        }
        return false;
    }

    void clear() {
        if(head == nullptr) return ;
        node *cur = head;
        while(cur->next != head) {
            node *del = cur->next;
            cur->next = cur->next->next;
            delete del;
        }
    }

    int size() const {
        int count = 0;
        node *cur = head;
        while(cur->next != head) {
            cur = cur->next;
            count ++ ;
        }
        return count;
    }
    
    // index begin from 0
    // if this index not valid then return false
    // else pass by reference to parameter val
    bool getValueByIndex(int index, string &val) const {
        node *cur = head;
        for(int i = 0; i <= index; i ++ ) {
            cur = cur->next;
            if(cur == head) return false;
        }
        val = cur->val;
        return true;
    }
    
    void travel() const {
        cout << "travel: ";
        node *cur = head;
        while(cur->next != head) {
            cout << cur->next->val << ' ';
            cur = cur->next;
        }
        cout << endl;
    }
    
    cycleList(const cycleList &link) {
        head = new node;
        head->next = head;
        int count = link.size();
        for(int i = count - 1; i >= 0; i -- ) {
            string s;
            link.getValueByIndex(i, s);
            add(s);
        }
    }
    
    cycleList& operator=(const cycleList &link) {
        clear();
        int count = link.size();
        for(int i = count - 1; i >= 0; i -- ) {
            string s;
            link.getValueByIndex(i, s);
            add(s);
        }
        return *this;
    }

    ~cycleList() {
        if(head == nullptr) return ;
        clear();
        delete head;
    }
};