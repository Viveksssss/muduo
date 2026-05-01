// #include "Router.h"
// #include "Logger.h"
// #include <sstream>
// #include <string>
// #include <unordered_map>

// void Router::addRoute(std::string const &path, std::string const &method,
//     std::string const &handler, std::vector<std::string> const &paramNames) {
//     auto current = _root;
//     auto segments = splitPath(path);
//     for (auto const &segment: segments) {
//         if (segment.empty()) {
//             continue;
//         }
//         if (segment[0] == ':') {
//             current->_paramNames.push_back(segment.substr(1));
//             if (!current->_children["*"]) {
//                 current->_children["*"] = std::make_shared<TrieNode>();
//             }
//             current = current->_children["*"];
//         } else {
//             if (!current->_children[segment]) {
//                 current->_children[segment] = std::make_shared<TrieNode>();
//             }
//             current = current->_children[segment];
//         }
//     }
//     current->isLeaf = true;
//     current->_handlers[method] = handler;
//     log_info("Added route: {} {} -> {}", method, path, handler);
// }

// RouteMatch Router::findRoute(std::string const &path, std::string const &method) {
//     log_info("Finding route for path : {}, method:{}", path, method);
//     // 1. 路径和查询参数分离
//     size_t queryPos = path.find('?');
//     std::string basePath = (queryPos == std::string::npos) ? path : path.substr(0, queryPos);
//     // 2. 查询参数
//     auto current = _root;
//     std::unordered_map<std::string, std::string> queryparams;
//     std::unordered_map<std::string, std::string> pathParams;

//     if (queryPos != std::string::npos && queryPos + 1 < path.size()) {
//         queryparams = parseQueryParams(path.substr(queryPos + 1));
//     }

//     std::vector<std::pair<std::shared_ptr<TrieNode>, std::unordered_map<std::string,
//     std::string>>>
//         matches;
//     auto segments = splitPath(basePath);

//     findMatches(current, segments, 0, pathParams, matches);
//     if (!matches.empty()) {
//         for (auto const &match: matches) {
//             if (match.first->isLeaf
//                 && match.first->_handlers.find(method) != match.first->_handlers.end()) {
//                 log_info("Found matching route with handler:{}", match.first->_handlers[method]);
//                 return RouteMatch{match.first->_handlers[method], match.second, queryparams};
//             }
//         }
//     }
//     log_info("No match route found");
//     return RouteMatch{"", {}};
// }

// std::unordered_map<std::string, std::string> Router::parseQueryParams(std::string const &query) {
//     if (query.empty()) {
//         return {};
//     }
//     std::unordered_map<std::string, std::string> params;
//     std::istringstream stream(query);
//     std::string pair;
//     while (std::getline(stream, pair, '&')) {
//         size_t eqPos = pair.find('=');
//         if (eqPos != std::string::npos) {
//             std::string key = pair.substr(0, eqPos);
//             std::string value = pair.substr(eqPos + 1);
//             params[key] = value;
//         }
//     }
//     return params;
// }

// std::vector<std::string> Router::splitPath(std::string const &path) {
//     std::vector<std::string> segments;
//     std::string segment;
//     std::istringstream pathStream(path);
//     while (std::getline(pathStream, segment, '/')) {
//         if (!segment.empty()) {
//             segments.push_back(segment);
//         }
//     }
//     return segments;
// }

// void Router::findMatches(std::shared_ptr<TrieNode> node, std::vector<std::string> const
// &segments,
//     size_t currentIndex, std::unordered_map<std::string, std::string> &currentParams,
//     std::vector<std::pair<std::shared_ptr<TrieNode>, std::unordered_map<std::string,
//     std::string>>>
//         &matches) {
//     if (currentIndex == segments.size()) {
//         if (node->isLeaf) {
//             matches.emplace_back(node, currentParams);
//         }
//         return;
//     }

//     std::string const &currentSegment = segments[currentIndex];
//     /* 1.精确匹配 */
//     if (node->_children.find(currentSegment) != node->_children.end()) {
//         findMatches(
//             node->_children[currentSegment], segments, currentIndex + 1, currentParams, matches);
//     }
//     /* 2.参数匹配 */
//     if (node->_children.find("*") != node->_children.end()) {
//         if (!node->_paramNames.empty()) {
//             auto paramNode = node->_children["*"];
//             currentParams[node->_paramNames.front()] = currentSegment;
//             findMatches(paramNode, segments, currentIndex + 1, currentParams, matches);
//             /* 回溯 */
//             currentParams.erase(node->_paramNames.front());
//         }
//     }
//     /* 3.通配符匹配 */
//     /*
//         findRoute("/static/css/style.css", "GET")
//         │
//         └─ findMatches(root_, segs, 0, {}, [])
//            │
//            ├─ 静态匹配 "static" → findMatches(staticNode, segs, 1, {}, [])
//            │  │
//            │  ├─ 静态匹配 "css": 无
//            │  ├─ 参数匹配 "*": 无
//            │  └─ 通配符匹配 "**":
//            │     │
//            │     ├─ i=1: 匹配0个段 → wildcardValue="" → 跳过
//            │     │
//            │     ├─ i=2: 匹配1个段 → wildcardValue="css"
//            │     │  └─ findMatches(wildcardNode, segs, 2, {"*":"css"}, [])
//            │     │     ├─ 终止? 否
//            │     │     ├─ 静态匹配 "style": 无
//            │     │     ├─ 参数匹配 "*": 无
//            │     │     └─ 通配符匹配 "**": 无
//            │     │     └─ 返回 []
//            │     │
//            │     ├─ i=3: 匹配2个段 → wildcardValue="css/style"
//            │     │  └─ findMatches(wildcardNode, segs, 3, {"*":"css/style"}, [])
//            │     │     ├─ 终止? 否
//            │     │     ├─ 静态匹配 "css": 无
//            │     │     ├─ 参数匹配 "*": 无
//            │     │     └─ 通配符匹配 "**": 无
//            │     │     └─ 返回 []
//            │     │
//            │     └─ i=4: 匹配3个段 → wildcardValue="css/style/css"
//            │        └─ findMatches(wildcardNode, segs, 4, {"*":"css/style/css"}, [])
//            │           ├─ 终止? 是 (4 >= 4)
//            │           ├─ node->isLeaf? 是
//            │           └─ matches.push_back({wildcardNode, {"*":"css/style/css"}})
//            │           └─ 返回 [{wildcardNode, {"*":"css/style/css"}}]
//            │
//            └─ 返回 [{wildcardNode, {"*":"css/style/css"}}]
//     */
//     if (node->_children.find("**") != node->_children.end()) {
//         auto wildcardNode = node->_children["**"];
//         std::string wildcardValue;
//         for (size_t i = currentIndex; i <= segments.size(); ++i) {
//             if (i > currentIndex) {
//                 if (!wildcardValue.empty()) {
//                     wildcardValue += "/";
//                 }
//                 wildcardValue += segments[i - 1];
//             } else {
//                 continue;
//             }

//             currentParams["*"] = wildcardValue;
//             findMatches(wildcardNode, segments, i, currentParams, matches);
//             currentParams.erase("*");
//         }
//         // for (size_t i = currentIndex; i <= segments.size(); ++i) {
//         //     std::string wildcardValue;
//         //     for (size_t j = currentIndex; j < i; ++j) {
//         //         if (!wildcardValue.empty()) {
//         //             wildcardValue += "/";
//         //         }
//         //         wildcardValue += segments[j];
//         //     }
//         //     if (!wildcardValue.empty()) {
//         //         currentParams["*"] = wildcardValue;
//         //         findMatches(wildcardNode, segments, i, currentParams, matches);
//         //         currentParams.erase("*");
//         //     }
//         // }
//     }
// }

// net/Router.cc
#include "Router.h"
#include "Logger.h"
#include <sstream>
#include <string>
#include <unordered_map>

// ==================== 添加路由 ====================

void Router::addRoute(std::string const &path, std::string const &method,
    std::string const &handler, std::vector<std::string> const &paramNames) {
    auto current = _root;
    auto segments = splitPath(path);

    for (auto const &segment: segments) {
        if (segment.empty()) {
            continue;
        }

        if (segment[0] == ':') {
            std::string paramName = segment.substr(1);

            // ==========================================
            // ✅ 修复：每个参数名用独立的 key
            //    ":id" 用 "*id"，":uid" 用 "*uid"
            //    这样不会互相污染
            // ==========================================
            std::string starKey = "*:" + paramName;

            if (!current->_children[starKey]) {
                current->_children[starKey] = std::make_shared<TrieNode>();
                current->_children[starKey]->_paramNames.push_back(paramName);
            }
            current = current->_children[starKey];

        } else if (segment == "**") {
            // 通配符
            if (!current->_children["**"]) {
                current->_children["**"] = std::make_shared<TrieNode>();
            }
            current = current->_children["**"];

        } else {
            // 静态节点
            if (!current->_children[segment]) {
                current->_children[segment] = std::make_shared<TrieNode>();
            }
            current = current->_children[segment];
        }
    }

    current->isLeaf = true;
    current->_handlers[method] = handler;
    log_info("Added route: {} {} -> {}", method, path, handler);
}

// ==================== 查找路由 ====================

RouteMatch Router::findRoute(std::string const &path, std::string const &method) {
    log_info("Finding route for path : {}, method:{}", path, method);

    // 1. 路径和查询参数分离
    size_t queryPos = path.find('?');
    std::string basePath = (queryPos == std::string::npos) ? path : path.substr(0, queryPos);

    // 2. 解析查询参数
    std::unordered_map<std::string, std::string> queryparams;
    if (queryPos != std::string::npos && queryPos + 1 < path.size()) {
        queryparams = parseQueryParams(path.substr(queryPos + 1));
    }

    // 3. 收集所有匹配
    std::unordered_map<std::string, std::string> pathParams;
    std::vector<std::pair<std::shared_ptr<TrieNode>, std::unordered_map<std::string, std::string>>>
        matches;
    auto segments = splitPath(basePath);

    findMatches(_root, segments, 0, pathParams, matches);

    // 4. 选择最佳匹配
    if (!matches.empty()) {
        for (auto const &match: matches) {
            if (match.first->isLeaf
                && match.first->_handlers.find(method) != match.first->_handlers.end()) {
                log_info("Found matching route with handler:{}", match.first->_handlers[method]);
                return RouteMatch{
                    match.first->_handlers[method],
                    match.second, // pathParams
                    queryparams   // queryParams
                };
            }
        }
    }

    log_info("No match route found");
    return RouteMatch{"", {}};
}

// ==================== 查询参数解析 ====================

std::unordered_map<std::string, std::string> Router::parseQueryParams(std::string const &query) {
    if (query.empty()) {
        return {};
    }

    std::unordered_map<std::string, std::string> params;
    std::istringstream stream(query);
    std::string pair;

    while (std::getline(stream, pair, '&')) {
        if (pair.empty()) {
            continue;
        }

        size_t eqPos = pair.find('=');
        if (eqPos != std::string::npos) {
            std::string key = pair.substr(0, eqPos);
            std::string value = pair.substr(eqPos + 1);
            params[key] = value;
        } else {
            // 无等号的参数，如 ?debug
            params[pair] = "";
        }
    }
    return params;
}

// ==================== 路径分割 ====================

std::vector<std::string> Router::splitPath(std::string const &path) {
    std::vector<std::string> segments;
    std::string segment;
    std::istringstream pathStream(path);

    while (std::getline(pathStream, segment, '/')) {
        if (!segment.empty()) {
            segments.push_back(segment);
        }
    }
    return segments;
}

// ==================== 递归查找匹配 ====================

void Router::findMatches(std::shared_ptr<TrieNode> node, std::vector<std::string> const &segments,
    size_t currentIndex, std::unordered_map<std::string, std::string> &currentParams,
    std::vector<std::pair<std::shared_ptr<TrieNode>, std::unordered_map<std::string, std::string>>>
        &matches) {
    if (currentIndex == segments.size()) {
        if (node->isLeaf) {
            matches.emplace_back(node, currentParams);
        }
        return;
    }

    std::string const &currentSegment = segments[currentIndex];

    // 1. 精确匹配
    if (node->_children.find(currentSegment) != node->_children.end()) {
        findMatches(
            node->_children[currentSegment], segments, currentIndex + 1, currentParams, matches);
    }

    // ==========================================
    // 2. 参数匹配：遍历所有 "*:xxx" 节点
    // ==========================================
    for (auto &[key, child]: node->_children) {
        if (key.size() >= 2 && key[0] == '*' && key[1] == ':') {
            // key 是 "*:id", "*:uid", "*:age" 等
            if (!child->_paramNames.empty()) {
                std::string paramName = child->_paramNames.front();

                currentParams[paramName] = currentSegment;
                findMatches(child, segments, currentIndex + 1, currentParams, matches);

                // 回溯
                currentParams.erase(paramName);
            }
        }
    }

    // 3. 通配符匹配 (**)
    if (node->_children.find("**") != node->_children.end()) {
        auto wildcardNode = node->_children["**"];
        std::string wildcardValue;

        for (size_t i = currentIndex; i <= segments.size(); ++i) {
            if (i > currentIndex) {
                if (!wildcardValue.empty()) {
                    wildcardValue += "/";
                }
                wildcardValue += segments[i - 1];
            } else {
                continue;
            }

            currentParams["*"] = wildcardValue;
            findMatches(wildcardNode, segments, i, currentParams, matches);
            currentParams.erase("*");
        }
    }
}
