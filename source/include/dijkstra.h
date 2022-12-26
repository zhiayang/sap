// dijkstra.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <ranges>
#include <algorithm> // for reverse
#include <functional>

#include "util.h" // for hasher

namespace util
{
	template <typename Node>
	concept DijkstraNode =
	    // clang-format off
	requires(Node node)
	{
		typename Node::Distance;

		// .neighbours should return an iterable of <distance, node> pairs
		{ node.neighbours() } -> std::ranges::range;
		{ *node.neighbours().begin() } -> std::convertible_to<const std::pair<Node, typename Node::Distance>&>;

		// Nodes should be hashable and equality comparable
		{ util::hasher{}(node) } -> std::convertible_to<size_t>;
		{ node == node } -> std::same_as<bool>;

		requires requires(typename Node::Distance d)
		{
			// distances should be comparable
			d <=> d;
			// distances should be default constructible to get a 0
			typename Node::Distance();
		};
	};
	// clang-format on

	template <DijkstraNode Node>
	std::vector<Node> dijkstra_shortest_path(Node start, Node end)
	{
		std::vector<Node> path {};
		using Distance = typename Node::Distance;
		// From, To, Distance from *start* to To, via From
		using Edge = std::tuple<Node, Node, Distance>;
		using EdgeComp = decltype([](const Edge& lhs, const Edge& rhs) {
			return std::get<Distance>(lhs) < std::get<Distance>(rhs);
		});

		// Use set instead of priority_queue so that we can .extract it
		std::multiset<Edge, EdgeComp> queue;
		// Maps Tos to Froms
		std::unordered_map<Node, Node, util::hasher> parents;

		queue.insert(Edge { start, start, Distance {} });
		while(!queue.empty())
		{
			auto [from, to, distance] = std::move(queue.extract(queue.begin()).value());
			if(!parents.try_emplace(to, from).second)
			{
				continue;
			}
			if(to == end)
			{
				path.push_back(to);
				auto node = to;
				while(true)
				{
					node = parents.find(node)->second;
					if(node == start)
					{
						break;
					}
					path.push_back(node);
				}
				// Reverse path
				std::reverse(path.begin(), path.end());
				return path;
			}

			// Not at end yet
			for(auto [neighbour, distance_to_neighbour] : to.neighbours())
			{
				queue.insert(Edge(to, neighbour, distance + distance_to_neighbour));
			}
		}
		// Unreachable if dijkstra is correctly implemented
		sap::internal_error("Fuck, dijkstra exploded");
	}
}
