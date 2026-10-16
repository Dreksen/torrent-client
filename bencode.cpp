#include "bencode.h"

namespace Bencode {
    Node ProcessInt(int &i, const std::string& s) {
        i++; // skip "i"

        Node NewNode = Node();
        NewNode.type = 0;
        NewNode.val = 0;

        long long sign = 1;
        if (s[i] == '-') {
            sign = -1;
            i++;
        }

        for (i; i < s.size() && s[i] != 'e'; i++) {
            if (s[i] < '0' || s[i] > '9') {
                throw std::runtime_error("wrong integer in " + std::to_string(i));
            }
            NewNode.val = NewNode.val * 10 + int(s[i] - '0');
        }

        if (sign == -1 && NewNode.val == 0) {
            throw std::runtime_error("found a negative zero in " + std::to_string(i));
        }

        NewNode.val *= sign;
        i++;

        return NewNode;
    }


    Node ProcessStr(int &i, const std::string& s) {
        int num = 0;
        for (i; i < s.size() && s[i] != ':'; i++) {
            if (s[i] < '0' || s[i] > '9') {
                throw std::runtime_error("wrong length of string " + std::to_string(i));
            }
            num = num * 10 + int(s[i] - '0');
        }

        i++; // pass ":"

        Node NewNode = Node();
        NewNode.type = 1;
        NewNode.str = "";

        if (i + num > s.size()) {
            throw std::runtime_error("processing string out of buf " + std::to_string(i));
        }
        NewNode.str = s.substr(i, num);
        
        i = i + num;

        return NewNode;
    }



    Node ProcessList(int &i, const std::string& s) {
        i++; // skip "l"
        Node NewNode = Node();
        NewNode.type = 2;

        while (i < s.size() && s[i] != 'e') {
            Node curRes = ProcessNode(i, s);
            NewNode.arr.push_back(curRes);
        }

        i++;
        return NewNode;
    }

    Node ProcessDict(int &i, const std::string& s) {
        i++; //skip 'd'
        Node NewNode = Node();
        NewNode.type = 3;

        Node val;

        while (i < s.size() && s[i] != 'e') {
            Node key = ProcessStr(i, s);
            Node val = ProcessNode(i, s);
            NewNode.map[key.str] = val;
        }

        i++;
        return NewNode;
    }


    Node ProcessNode(int &i, const std::string& s) {
        if (i >= s.size()) throw std::runtime_error("unexpected end of file " + std::to_string(i));
        
        if (s[i] == 'i') return ProcessInt(i, s);
        if (isdigit(s[i])) return ProcessStr(i, s);
        if (s[i] == 'l') return ProcessList(i, s);
        if (s[i] == 'd') return ProcessDict(i, s);
        
        throw std::runtime_error("invalid type " + std::to_string(i));
    }
}
