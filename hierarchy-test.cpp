#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include "hierarchy/entity.hpp"

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

// hy::display_ops_t display_ops;
// display_ops.set_bold_fn = [](const bool bold) {
// };
// display_ops.set_invert_fn = [](const bool invert) {
// };
// display_ops.draw_fn = [](const std::string_view str) {
// };
// display_ops.draw_at_fn =
//   [](const int x, const int y, const std::string_view str) {
//   };

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
}
