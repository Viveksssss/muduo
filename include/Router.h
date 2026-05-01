// #pragma once
// #include "copyable.h"
// #include <memory>
// #include <string>
// #include <unordered_map>
// #include <vector>

// struct RouteMatch {
//     std::string handler;
//     std::unordered_map<std::string, std::string> pathParams;
//     std::unordered_map<std::string, std::string> queryParams;
// };

// class TrieNode {
// public:
//     std::unordered_map<std::string, std::shared_ptr<TrieNode>> _children;
//     /* method->handler */
//     std::unordered_map<std::string, std::string> _handlers;
//     std::vector<std::string> _paramNames;
//     bool isLeaf;

//     TrieNode() : isLeaf(false) { }
// };

// class Router : public copyable {
// public:
//     Router() : _root(std::make_shared<TrieNode>()) { }

//     void addRoute(std::string const &path, std::string const &method, std::string const &handler,
//         std::vector<std::string> const &paramNames = {});

//     RouteMatch findRoute(std::string const &path, std::string const &method);

//     void clear() {
//         std::make_shared<TrieNode>().swap(_root);
//     }

// private:
//     std::unordered_map<std::string, std::string> parseQueryParams(std::string const &query);
//     std::vector<std::string> splitPath(std::string const &path);
//     // clang-format off
//     void findMatches(
//         std::shared_ptr<TrieNode> node,                              // 当前节点
//         std::vector<std::string> const &segments,                    // 路径分段
//         size_t currentIndex,                                         // 当前分段索引
//         std::unordered_map<std::string, std::string>&currentParams,  // 当前路径参数
//         std::vector<std::pair<std::shared_ptr<TrieNode>, std::unordered_map<std::string,
//         std::string>>> &matches
//         // 输出:所有匹配的节点和对应的路径参数
//     );
//     // clang-format on

// private:
//     std::shared_ptr<TrieNode> _root;
// };

// net/Router.h
#pragma once

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

struct RouteMatch {
    std::string handler;
    std::unordered_map<std::string, std::string> pathParams;
    std::unordered_map<std::string, std::string> queryParams;

    RouteMatch() = default;

    RouteMatch(std::string h, std::unordered_map<std::string, std::string> pp,
        std::unordered_map<std::string, std::string> qp = {})
        : handler(std::move(h))
        , pathParams(std::move(pp))
        , queryParams(std::move(qp)) { }
};

class TrieNode {
public:
    std::map<std::string, std::shared_ptr<TrieNode>> _children;
    std::map<std::string, std::string> _handlers;
    std::vector<std::string> _paramNames;
    bool isLeaf;

    TrieNode() : isLeaf(false) { }
};

class Router {
public:
    Router() : _root(std::make_shared<TrieNode>()) { }

    void addRoute(std::string const &path, std::string const &method, std::string const &handler,
        std::vector<std::string> const &paramNames = {});

    RouteMatch findRoute(std::string const &path, std::string const &method);

    void clear() {
        _root = std::make_shared<TrieNode>();
    }

private:
    std::shared_ptr<TrieNode> _root;
    std::vector<std::string> splitPath(std::string const &path);
    std::unordered_map<std::string, std::string> parseQueryParams(std::string const &query);

    void findMatches(std::shared_ptr<TrieNode> node, std::vector<std::string> const &segments,
        size_t currentIndex, std::unordered_map<std::string, std::string> &currentParams,
        std::vector<std::pair<std::shared_ptr<TrieNode>,
            std::unordered_map<std::string, std::string>>> &matches);
};
