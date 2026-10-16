#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <variant>
#include <list>
#include <map>
#include <sstream>

namespace Bencode {

/*
 * В это пространство имен рекомендуется вынести функции для работы с данными в формате bencode.
 * Этот формат используется в .torrent файлах и в протоколе общения с трекером
 */

    struct Node {
        int type;  // 0 int, 1 string, 2 list, 3 map
        long long val;
        std::string str;
        std::vector<Node> arr;
        std::map<std::string, Node> map;
    };

    Node ProcessInt(int &i, const std::string& s);
    Node ProcessStr(int &i, const std::string& s);
    Node ProcessList(int &i, const std::string& s);
    Node ProcessDict(int &i, const std::string& s);
    Node ProcessNode(int &i, const std::string& s);
}
