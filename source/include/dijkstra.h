// dijkstra.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <functional>
#include <ranges>
#include <queue>

#include "defs.h"

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
		using Distance = typename Node::Distance;
		using DistanceNodePair = std::pair<Node, Distance>;
		using DistanceComp = decltype([](const DistanceNodePair& lhs, const DistanceNodePair& rhs) {
			return lhs.second > rhs.second;
		});

		std::priority_queue<DistanceNodePair, std::vector<DistanceNodePair>, DistanceComp> queue;
		std::unordered_map<Node, Distance, util::hasher> distances;

		queue.push(DistanceNodePair { start, Distance {} });

		return {};
	}

}
