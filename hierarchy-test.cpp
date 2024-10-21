#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include "hierarchy/entity.hpp"

#include <unordered_map>
#include <utility>

// helper function to reduce boilerplate
template<typename Fn>
void repeat_n(size_t n, Fn&& fn) {
  for (size_t i = 0; i < n; i++) {
    fn();
  }
}

template<typename Fn>
void repeat_n_it(size_t n, Fn&& fn) {
  for (size_t i = 0; i < n; i++) {
    fn(i);
  }
}

namespace thh {
  doctest::String toString(const thh::handle_t& handle) {
    return doctest::String("handle{") + doctest::toString(handle.id_)
         + doctest::String(", ") + doctest::toString(handle.gen_)
         + doctest::String("}");
  }

  doctest::String toString(const std::vector<thh::handle_t>& handles) {
    return [&, str = doctest::String()]() mutable {
      str += "{";
      std::for_each(handles.begin(), handles.end() - 1, [&](const auto handle) {
        str += toString(handle);
        str += ", ";
      });
      str += toString(handles.back()) + "}";
      return str;
    }();
  }
} // namespace thh

TEST_CASE("Hierarchy Traversal") {
  thh::handle_vector_t<hy::entity_t> entities;
  auto root_handles = demo::create_sample_entities(entities);

  hy::interaction_t interaction;
  hy::collapser_t collapser;

  SUBCASE("move_down on last element moves to child if reachable") {
    interaction.select(thh::handle_t(0, 0), entities, root_handles);

    demo::process_input(
      demo::input_e::move_down, entities, root_handles, interaction);

    CHECK(interaction.selected() == thh::handle_t(1, 0));
    CHECK(interaction.element() == 0);
    CHECK(
      interaction.siblings()
      == entities
           .call_return(
             thh::handle_t(0, 0),
             [](const hy::entity_t& entity) { return entity.children_; })
           .value_or(std::vector<thh::handle_t>{}));
  }

  SUBCASE("move_down on first element moves to sibling if reachable") {
    interaction.select(thh::handle_t(1, 0), entities, root_handles);

    demo::process_input(
      demo::input_e::move_down, entities, root_handles, interaction);

    CHECK(interaction.selected() == thh::handle_t(2, 0));
    CHECK(interaction.element() == 1);
    CHECK(
      interaction.siblings()
      == hy::siblings(interaction.selected(), entities, root_handles));
  }

  SUBCASE("move_down on second element moves to child if reachable") {
    interaction.select(thh::handle_t(2, 0), entities, root_handles);

    demo::process_input(
      demo::input_e::move_down, entities, root_handles, interaction);

    CHECK(interaction.selected() == thh::handle_t(5, 0));
    CHECK(interaction.element() == 0);
    CHECK(
      interaction.siblings()
      == hy::siblings(interaction.selected(), entities, root_handles));
  }

  SUBCASE(
    "move_down on last element moves to next parent sibling if reachable") {
    interaction.select(thh::handle_t(10, 0), entities, root_handles);

    demo::process_input(
      demo::input_e::move_down, entities, root_handles, interaction);

    CHECK(interaction.selected() == thh::handle_t(11, 0));
    CHECK(interaction.element() == 2);
    CHECK(
      interaction.siblings()
      == hy::siblings(interaction.selected(), entities, root_handles));
  }

  SUBCASE(
    "move_down on last element moves to root parent sibling if reachable") {
    interaction.select(thh::handle_t(11, 0), entities, root_handles);

    demo::process_input(
      demo::input_e::move_down, entities, root_handles, interaction);

    CHECK(interaction.selected() == thh::handle_t(7, 0));
    CHECK(interaction.element() == 1);
    CHECK(interaction.siblings() == root_handles);
  }

  SUBCASE("move_down on final element has no effect") {
    interaction.select(thh::handle_t(9, 0), entities, root_handles);

    demo::process_input(
      demo::input_e::move_down, entities, root_handles, interaction);

    CHECK(interaction.selected() == thh::handle_t(9, 0));
    CHECK(interaction.element() == 0);
    CHECK(
      interaction.siblings()
      == hy::siblings(interaction.selected(), entities, root_handles));
  }

  SUBCASE("move_up on first element has no effect") {
    interaction.select(thh::handle_t(0, 0), entities, root_handles);

    demo::process_input(
      demo::input_e::move_up, entities, root_handles, interaction);

    CHECK(interaction.selected() == thh::handle_t(0, 0));
    CHECK(interaction.element() == 0);
    CHECK(
      interaction.siblings()
      == hy::siblings(interaction.selected(), entities, root_handles));
  }

  SUBCASE("move_up on first child goes to parent") {
    interaction.select(thh::handle_t(1, 0), entities, root_handles);

    demo::process_input(
      demo::input_e::move_up, entities, root_handles, interaction);

    CHECK(interaction.selected() == thh::handle_t(0, 0));
    CHECK(interaction.element() == 0);
    CHECK(
      interaction.siblings()
      == hy::siblings(interaction.selected(), entities, root_handles));
  }

  SUBCASE("move_up on later sibling goes to sibling") {
    interaction.select(thh::handle_t(6, 0), entities, root_handles);

    demo::process_input(
      demo::input_e::move_up, entities, root_handles, interaction);

    CHECK(interaction.selected() == thh::handle_t(5, 0));
    CHECK(interaction.element() == 0);
    CHECK(
      interaction.siblings()
      == hy::siblings(interaction.selected(), entities, root_handles));
  }

  SUBCASE("move_up on expanded sibling goes to sibling child") {
    interaction.select(thh::handle_t(11, 0), entities, root_handles);

    demo::process_input(
      demo::input_e::move_up, entities, root_handles, interaction);

    CHECK(interaction.selected() == thh::handle_t(10, 0));
    CHECK(interaction.element() == 0);
    CHECK(
      interaction.siblings()
      == hy::siblings(interaction.selected(), entities, root_handles));
  }

  SUBCASE("move_up on expanded sibling goes to most expanded sibling child") {
    interaction.select(thh::handle_t(7, 0), entities, root_handles);

    demo::process_input(
      demo::input_e::move_up, entities, root_handles, interaction);

    CHECK(interaction.selected() == thh::handle_t(11, 0));
    CHECK(interaction.element() == 2);
    CHECK(
      interaction.siblings()
      == hy::siblings(interaction.selected(), entities, root_handles));
  }

  SUBCASE("move_up on expanded sibling goes to sibling with collapsed child") {
    interaction.select(thh::handle_t(7, 0), entities, root_handles);
    interaction.collapse(thh::handle_t(2, 0), entities);

    demo::process_input(
      demo::input_e::move_up, entities, root_handles, interaction);

    CHECK(interaction.selected() == thh::handle_t(2, 0));
    CHECK(interaction.element() == 1);
    CHECK(
      interaction.siblings()
      == hy::siblings(interaction.selected(), entities, root_handles));
  }

  SUBCASE("move_down on collapsed element goes to next sibling") {
    interaction.select(thh::handle_t(0, 0), entities, root_handles);
    interaction.collapse(thh::handle_t(0, 0), entities);

    demo::process_input(
      demo::input_e::move_down, entities, root_handles, interaction);

    CHECK(interaction.selected() == thh::handle_t(7, 0));
    CHECK(interaction.element() == 1);
    CHECK(interaction.siblings() == root_handles);
  }
}

TEST_CASE("Scrollable Hierarchy Traversal") {
  thh::handle_vector_t<hy::entity_t> entities;

  const auto handle = entities.add();
  entities.call(handle, [handle](auto& entity) {
    entity.name_ = std::string("entity_") + std::to_string(handle.id_);
  });

  hy::collapser_t collapser;
  std::vector<thh::handle_t> root_handles(1, handle);

  hy::view_t view(
    hy::flatten_entities(entities, collapser, root_handles), 0, 10);

  SUBCASE("single entity can be deleted") {
    view.remove(entities, collapser, root_handles);
    CHECK(view.flattened_handles().empty());
    CHECK(root_handles.empty());
    CHECK(entities.empty());

    SUBCASE("no entities gives empty selection") {
      CHECK(view.selected_handle() == thh::handle_t());
    }

    SUBCASE("sibling can be added when there are no entities") {
      const auto added = view.add_sibling(entities, collapser, root_handles);

      CHECK(added.index_ == 0);
      CHECK(added.flattened_handle_.indent_ == 0);

      CHECK(view.selected_index() == 0);
      CHECK(view.selected_handle() == root_handles.front());
    }

    SUBCASE("child cannot be added when there are no entities") {
      const auto added = view.add_child(entities, collapser);
      CHECK(added == std::nullopt);
    }
  }

  SUBCASE(
    "offset updated to show last handle if all handles are deleted offscreen") {
    repeat_n(10, [&] { view.add_sibling(entities, collapser, root_handles); });
    repeat_n(10, [&] { view.move_down(); });
    CHECK(view.offset() == 1);
    repeat_n(10, [&] { view.remove(entities, collapser, root_handles); });
    // should have moved offset back to 0 to show last remaining handle
    CHECK(view.offset() == 0);
  }

  SUBCASE("single entity is selected entity") {
    CHECK(view.selected_index() == 0);
    CHECK(view.selected_handle() == root_handles.front());
  }

  SUBCASE("single entity move up has no effect") {
    view.move_up();

    CHECK(view.selected_index() == 0);
    CHECK(view.selected_handle() == root_handles.front());
  }

  SUBCASE("single entity move down has no effect") {
    view.move_down();

    CHECK(view.selected_index() == 0);
    CHECK(view.selected_handle() == root_handles.front());
  }

  SUBCASE("child added with correct offset and indentation") {
    const auto added = view.add_child(entities, collapser);

    CHECK(added->flattened_handle_.indent_ == 1);
    CHECK(added->index_ == 1);

    SUBCASE("move down moves to child entity") {
      view.move_down();

      CHECK(view.selected_handle() == added->flattened_handle_.entity_handle_);
      CHECK(view.selected_indent() == added->flattened_handle_.indent_);
      CHECK(view.selected_index() == added->index_);
    }

    SUBCASE("collapse parent and move down has no effect") {
      view.collapse(entities, collapser);
      view.move_down();

      CHECK(view.selected_index() == 0);
      CHECK(view.selected_handle() == root_handles.front());
    }

    SUBCASE("collapse reduces size of flattened_handles") {
      CHECK(view.flattened_handles().size() == 2);
      view.collapse(entities, collapser);
      CHECK(view.flattened_handles().size() == 1);
    }

    SUBCASE("sibling added below child") {
      const auto added = view.add_sibling(entities, collapser, root_handles);

      CHECK(added.index_ == 2);
      CHECK(added.flattened_handle_.indent_ == 0);

      SUBCASE("sibling added below added sibling") {
        const auto added_again =
          view.add_sibling(entities, collapser, root_handles);

        CHECK(added_again.index_ == 3);
        CHECK(added_again.flattened_handle_.indent_ == 0);
      }
    }
  }

  SUBCASE("flattened handles resized after collapse") {
    repeat_n(10, [&] { view.add_child(entities, collapser); });
    CHECK(view.flattened_handles().size() == 11);
    view.collapse(entities, collapser);
    CHECK(view.flattened_handles().size() == 1);

    SUBCASE("flattened handles resized after expand") {
      view.expand(entities, collapser);
      CHECK(view.flattened_handles().size() == 11);

      SUBCASE("sibling added at correct offset when children expanded") {
        auto position = view.add_sibling(entities, collapser, root_handles);
        CHECK(position.index_ == 11);
        CHECK(position.flattened_handle_.entity_handle_.id_ == 11);

        SUBCASE("sibling offset updated when children collapsed") {
          view.collapse(entities, collapser);
          view.move_down();
          CHECK(view.selected_handle().id_ == 11);
          CHECK(view.selected_index() == 1);
        }
      }
    }
  }

  SUBCASE("child indents increase as hierarchy depth grows") {
    for (int i = 0; i < 5; ++i) {
      auto added_child = view.add_child(entities, collapser);
      CHECK(added_child->flattened_handle_.indent_ == i + 1);
      view.move_down();
    }
  }
}

template<class T>
inline void hash_combine(std::size_t& seed, const T& v) {
  std::hash<T> hasher;
  seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

struct pair_hash {
  template<class T1, class T2>
  std::size_t operator()(const std::pair<T1, T2>& pair) const {
    size_t seed = 0;
    hash_combine(seed, pair.first);
    hash_combine(seed, pair.second);
    return seed;
  }
};

TEST_CASE("Scrollable Hierarchy Display") {
  bool bolded = false;
  bool inverted = false;
  int last_x = 0;
  int last_y = 0;
  std::unordered_map<std::pair<int, int>, std::string, pair_hash> values;

  hy::display_ops_t display_ops;
  display_ops.connection_ = "|";
  display_ops.end_ = "L";
  display_ops.mid_ = "-";
  display_ops.indent_width_ = 1;

  display_ops.set_bold_fn_ = [&bolded](const bool bold) { bolded = bold; };
  display_ops.set_invert_fn_ = [&inverted](const bool invert) {
    inverted = invert;
  };
  display_ops.draw_fn_ = [&values, &last_x,
                          &last_y](const std::string_view str) {
    auto inserted =
      values.insert({std::pair(last_x, last_y), std::string(str)});
    CHECK(inserted.second);
    last_x += str.size();
  };
  display_ops.draw_at_fn_ =
    [&values, &last_x,
     &last_y](const int x, const int y, const std::string_view str) {
      auto inserted = values.insert({std::pair(x, y), std::string(str)});
      CHECK(inserted.second);
      last_x = x + str.size();
      last_y = y;
    };

  thh::handle_vector_t<hy::entity_t> entities;

  const auto handle = entities.add();
  entities.call(handle, [handle](auto& entity) {
    entity.name_ = std::string("entity_") + std::to_string(handle.id_);
  });

  hy::collapser_t collapser;
  std::vector<thh::handle_t> root_handles(1, handle);

  hy::view_t view(
    hy::flatten_entities(entities, collapser, root_handles), 0, 10);

  SUBCASE("first handle drawn with end when size is one") {
    hy::display_scrollable_hierarchy(
      entities, root_handles, view, collapser, display_ops);
    CHECK(values[std::pair(0, 0)] == display_ops.end_);
    CHECK(values.size() == 2);
  }

  SUBCASE("first handle drawn with mid second handle drawn with end when size "
          "is two") {
    view.add_sibling(entities, collapser, root_handles);
    hy::display_scrollable_hierarchy(
      entities, root_handles, view, collapser, display_ops);
    CHECK(values[std::pair(0, 0)] == display_ops.mid_);
    CHECK(values[std::pair(0, 1)] == display_ops.end_);
  }

  SUBCASE("third handle drawn with end when child of first handle") {
    view.add_sibling(entities, collapser, root_handles);
    view.add_child(entities, collapser);
    hy::display_scrollable_hierarchy(
      entities, root_handles, view, collapser, display_ops);
    CHECK(values[std::pair(0, 0)] == display_ops.mid_);
    CHECK(values[std::pair(0, 2)] == display_ops.end_);
    CHECK(values[std::pair(1, 1)] == display_ops.end_);
  }

  SUBCASE("last visible handle not drawn with end when sibling offscreen") {
    repeat_n(10, [&] { view.add_sibling(entities, collapser, root_handles); });
    hy::display_scrollable_hierarchy(
      entities, root_handles, view, collapser, display_ops);
    CHECK(values[std::pair(0, 9)] == display_ops.mid_);
  }

  SUBCASE("last visible handle drawn with end when scrolled into view") {
    repeat_n(10, [&] { view.add_sibling(entities, collapser, root_handles); });
    repeat_n(10, [&] { view.move_down(); });
    hy::display_scrollable_hierarchy(
      entities, root_handles, view, collapser, display_ops);
    CHECK(values[std::pair(0, 9)] == display_ops.end_);
  }

  SUBCASE("linking connection displayed between siblings, first with child") {
    view.add_sibling(entities, collapser, root_handles);
    view.add_child(entities, collapser);
    hy::display_scrollable_hierarchy(
      entities, root_handles, view, collapser, display_ops);
    CHECK(values[std::pair(0, 1)] == display_ops.connection_);
  }

  SUBCASE("linking connection expanded between siblings as children added") {
    view.add_sibling(entities, collapser, root_handles);
    view.add_child(entities, collapser);
    view.move_down();
    repeat_n(2, [&] { view.add_sibling(entities, collapser, root_handles); });
    hy::display_scrollable_hierarchy(
      entities, root_handles, view, collapser, display_ops);
    CHECK(values[std::pair(0, 1)] == display_ops.connection_);
    CHECK(values[std::pair(0, 2)] == display_ops.connection_);
    CHECK(values[std::pair(0, 3)] == display_ops.connection_);
  }

  SUBCASE("linking connection repeated at each level as children added") {
    repeat_n(3, [&] {
      view.add_sibling(entities, collapser, root_handles);
      view.add_child(entities, collapser);
      view.move_down();
    });
    hy::display_scrollable_hierarchy(
      entities, root_handles, view, collapser, display_ops);
    repeat_n_it(5, [&](size_t i) {
      const int offset = 1;
      CHECK(values[std::pair(0, i + offset)] == display_ops.connection_);
    });
    repeat_n_it(3, [&](size_t i) {
      const int offset = 2;
      CHECK(values[std::pair(1, i + offset)] == display_ops.connection_);
    });
    CHECK(values[std::pair(2, 3)] == display_ops.connection_);
  }

  SUBCASE("linking connection present for sibling offscreen below") {
    repeat_n(9, [&] { view.add_child(entities, collapser); });
    view.add_sibling(entities, collapser, root_handles);
    hy::display_scrollable_hierarchy(
      entities, root_handles, view, collapser, display_ops);
    repeat_n_it(9, [&](size_t i) {
      const int offset = 1;
      CHECK(values[std::pair(0, i + offset)] == display_ops.connection_);
    });
  }

  SUBCASE(
    "linking connection present for sibling offscreen below with offset") {
    repeat_n(9, [&] { view.add_child(entities, collapser); });
    view.add_sibling(entities, collapser, root_handles);
    repeat_n(10, [&] { view.move_down(); });
    repeat_n(9, [&] { view.add_child(entities, collapser); });
    view.add_sibling(entities, collapser, root_handles);
    repeat_n(9, [&] { view.move_down(); });
    hy::display_scrollable_hierarchy(
      entities, root_handles, view, collapser, display_ops);
    repeat_n_it(9, [&](size_t i) {
      const int offset = 1;
      CHECK(values[std::pair(0, i + offset)] == display_ops.connection_);
    });
  }

  SUBCASE(
    "linking connection present for sibling offscreen above with offset") {
    repeat_n(9, [&] { view.add_child(entities, collapser); });
    view.add_sibling(entities, collapser, root_handles);
    repeat_n(10, [&] { view.move_down(); });
    repeat_n(9, [&] { view.add_child(entities, collapser); });
    view.add_sibling(entities, collapser, root_handles);
    repeat_n(10, [&] { view.move_down(); });
    hy::display_scrollable_hierarchy(
      entities, root_handles, view, collapser, display_ops);
    repeat_n_it(9, [&](size_t i) {
      CHECK(values[std::pair(0, i)] == display_ops.connection_);
    });
  }

  SUBCASE("linking connection present for sibling offscreen below with offset "
          "and indent") {
    repeat_n(9, [&] { view.add_child(entities, collapser); });
    view.add_sibling(entities, collapser, root_handles);
    repeat_n(9, [&] { view.move_down(); });
    repeat_n(9, [&] { view.add_child(entities, collapser); });
    view.add_sibling(entities, collapser, root_handles);
    repeat_n(9, [&] { view.move_down(); });
    hy::display_scrollable_hierarchy(
      entities, root_handles, view, collapser, display_ops);
    repeat_n_it(9, [&](size_t i) {
      const int offset = 1;
      CHECK(values[std::pair(1, i + offset)] == display_ops.connection_);
    });
  }

  SUBCASE("linking connections drawn for offscreen handles") {
    repeat_n(10, [&] { view.add_child(entities, collapser); });
    view.add_sibling(entities, collapser, root_handles);
    repeat_n(10, [&] { view.move_down(); });
    hy::display_scrollable_hierarchy(
      entities, root_handles, view, collapser, display_ops);
    repeat_n_it(9, [&](size_t i) {
      CHECK(values[std::pair(0, i)] == display_ops.connection_);
    });
  }

  // create a 'triangle shape' hierarchy, see top half, then move down (scroll)
  // to see bottom half
  SUBCASE("all types of linking connections drawn for handles") {
    repeat_n(9, [&] {
      view.add_sibling(entities, collapser, root_handles);
      view.add_child(entities, collapser);
      view.move_down();
    });
    hy::display_scrollable_hierarchy(
      entities, root_handles, view, collapser, display_ops);
    for (int c = 0, s = 1; c < 9; ++c, ++s) {
      for (int r = s; r < 9; r++) {
        CHECK(values[std::pair(c, r)] == display_ops.connection_);
      }
    }
    repeat_n(9, [&] { view.move_down(); });
    values.clear();
    hy::display_scrollable_hierarchy(
      entities, root_handles, view, collapser, display_ops);
    for (int c = 0, e = 9; c < 9; ++c, --e) {
      for (int r = 0; r < e; r++) {
        CHECK(values[std::pair(c, r)] == display_ops.connection_);
      }
    }
  }

  SUBCASE("ensure certain linking connections are not drawn") {
    view.add_sibling(entities, collapser, root_handles);
    repeat_n(2, [&] { view.add_child(entities, collapser); });
    repeat_n(2, [&] { view.move_down(); });
    repeat_n(3, [&] { view.add_child(entities, collapser); });
    repeat_n(2, [&] { view.move_down(); });
    view.add_child(entities, collapser);
    repeat_n(3, [&] { view.move_down(); });
    repeat_n(2, [&] { view.add_child(entities, collapser); });
    hy::display_scrollable_hierarchy(
      entities, root_handles, view, collapser, display_ops);
    repeat_n_it(4, [&](size_t i) {
      CHECK(values.find(std::pair(1, i + 3)) == values.end());
    });
    repeat_n_it(2, [&](size_t i) {
      CHECK(values.find(std::pair(0, i + 8)) == values.end());
    });
    CHECK(values.size() == 27);
  }

  SUBCASE(
    "ensure next sibling linking connections are not drawn out of bounds") {
    view.add_sibling(entities, collapser, root_handles);
    repeat_n(15, [&] { view.add_child(entities, collapser); });
    hy::display_scrollable_hierarchy(
      entities, root_handles, view, collapser, display_ops);
    repeat_n_it(5, [&](size_t i) {
      const int offset = 10;
      CHECK(values.find(std::pair(0, offset + i)) == values.end());
    });
  }
}
