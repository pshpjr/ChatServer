#pragma once

#include <array>
#include <list>
#include <map>
#include <queue>
#include <set>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>


template <typename Type, unsigned int Size>
using Array = std::array<Type, Size>;

template <typename Type>
using Vector = std::vector<Type>;

template <typename Type>
using List = std::list<Type>;

template <typename Key, typename Type, typename Pred = std::less<Key>>
using Map = std::map<Key, Type, Pred>;

template <typename Key, typename Pred = std::less<Key>>
using Set = std::set<Key, Pred>;

template <typename Type>
using Deque = std::deque<Type>;

template <typename Type, typename Container = Deque<Type>>
using Queue = std::queue<Type, Container>;

template <typename Type, typename Container = Deque<Type>>
using Stack = std::stack<Type, Container>;

template <typename Type, typename Container = Vector<Type>, typename Pred = std::less<typename Container::value_type>>
using PriorityQueue = std::priority_queue<Type, Container, Pred>;

using String = std::basic_string<wchar_t, std::char_traits<wchar_t>>;

template <typename Key, typename Type, typename Hasher = std::hash<Key>, typename KeyEq = std::equal_to<Key>>
using HashMap = std::unordered_map<Key, Type, Hasher, KeyEq>;

template <typename Key, typename Hasher = std::hash<Key>, typename KeyEq = std::equal_to<Key>>
using HashSet = std::unordered_set<Key, Hasher, KeyEq>;


