#pragma once

// 文字列
#include <string>        // std::string, std::wstring (DX12のパス指定等で多用)
#include <string_view>   // std::string_view (読み取り専用文字列の軽量な参照)
#include <sstream>       // std::stringstream (文字列の組み立て)
#include <format>        // std::format (C++20: printfに代わる強力な書式化)

// コンテナ
#include <vector>        // std::vector (頂点データ、コマンドリスト、リソース管理の主力)
#include <array>         // std::array (固定長配列。RootSignatureの記述等に)
#include <list>          // std::list (要素の頻繁な挿入・削除用)
#include <deque>         // std::deque (双方向キュー)
#include <map>           // std::map (リソース名の管理。二分木)
#include <unordered_map> // std::unordered_map (ハッシュマップ。高速な検索用)
#include <set>           // std::set (重複のない要素集合)
#include <unordered_set> // std::unordered_set (ハッシュ版set)
#include <queue>         // std::queue, std::priority_queue (タスクキュー等)
#include <stack>         // std::stack (LIFO構造)
#include <span>          // std::span (C++20: 連続したメモリ領域 [ポインタ+サイズ] の安全な参照)

// アルゴリズム・数値
#include <algorithm>     // std::sort, std::find, std::min/max (演算の基本)
#include <numeric>       // std::iota, std::accumulate (数値配列の集計・生成)
#include <cmath>         // sin, cos, sqrt (数学演算)
#include <numbers>       // std::numbers::pi (C++20: 円周率などの定数)
#include <random>        // std::mt19937, std::uniform_real_distribution (乱数生成)
#include <limits>        // std::numeric_limits (型の最大値・最小値)
#include <bit>           // std::bit_cast, std::endian (C++20: ビット操作)
						 
// メモリ				   
#include <memory>        // std::unique_ptr, std::shared_ptr (リソースの寿命管理)
#include <optional>      // std::optional (値があるか空かを表す型)
#include <variant>       // std::variant (型の安全な共用体)
#include <any>           // std::any (任意の型を保持できる型)
						 
// 入出力				   
#include <iostream>      // std::cout (デバッグログ出力)
#include <fstream>       // std::ifstream, std::ofstream (シェーダーバイナリや設定ファイルの読み込み)
#include <filesystem>    // std::filesystem::path (ファイルパス操作、ディレクトリ走査)
						 
// ユーティリティ			 
#include <functional>    // std::function, std::bind (コールバック、関数オブジェクト)
#include <utility>       // std::move, std::pair, std::make_pair (右辺値参照やペア)
#include <tuple>         // std::tuple (複数の型の組)
#include <chrono>        // std::chrono (高精度タイマー。FPS計測やデルタタイム算出)
#include <cassert>       // assert (デバッグ時の想定外チェック)
#include <stdexcept>     // std::runtime_error (例外処理)
#include <typeinfo>      // typeid (型情報の取得)
#include <concepts>      // std::derived_from, std::integral (C++20: テンプレートの制約)
#include <ranges>        // std::ranges (C++20: パイプライン記法によるコンテナ操作)