#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include "hierarchy/entity.hpp"

#include <unordered_map>
#include <utility>

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
  thh::container_t<hy::entity_t> entities;
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
  thh::container_t<hy::entity_t> entities;

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
    CHECK(entities.size() == 0);

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
    for (int i = 0; i < 10; ++i) {
      view.add_sibling(entities, collapser, root_handles);
    }
    for (int i = 0; i < 10; ++i) {
      view.move_down();
    }
    CHECK(view.offset() == 1);
    for (int i = 0; i < 10; ++i) {
      view.remove(entities, collapser, root_handles);
    }
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
    for (int i = 0; i < 10; ++i) {
      view.add_child(entities, collapser);
    }
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
  display_ops.set_bold_fn_ = [&bolded](const bool bold) { bolded = bold; };
  display_ops.set_invert_fn_ = [&inverted](const bool invert) {
    inverted = invert;
  };
  display_ops.draw_fn_ = [&values, &last_x,
                          &last_y](const std::string_view str) {
    values[std::pair(last_x, last_y)] = str;
    last_x += str.size();
  };
  display_ops.draw_at_fn_ =
    [&values, &last_x,
     &last_y](const int x, const int y, const std::string_view str) {
      values[std::pair(x, y)] = str;
      last_x = x + str.size();
      last_y = y;
    };

  thh::container_t<hy::entity_t> entities;

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
  }

  SUBCASE("first handle drawn with mid second handle drawn with end when size "
          "is two") {
    view.add_sibling(entities, collapser, root_handles);
    hy::display_scrollable_hierarchy(
      entities, root_handles, view, collapser, display_ops);
    CHECK(values[std::pair(0, 0)] == display_ops.mid_);
    CHECK(values[std::pair(0, 1)] == display_ops.end_);
  }
}
