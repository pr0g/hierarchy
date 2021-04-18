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
    return [&, str = doctest::String()] () mutable {
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

  SUBCASE("move_down on last element moves to child if reachable") {
    interaction.element_ = 0;
    interaction.selected_ = thh::handle_t(0, 0);
    interaction.siblings_ = root_handles;

    demo::process_input(
      demo::input_e::move_down, entities, root_handles, interaction);

    CHECK(interaction.selected_ == thh::handle_t(1, 0));
    CHECK(interaction.element_ == 0);
    CHECK(
      interaction.siblings_
      == entities
           .call_return(
             thh::handle_t(0, 0),
             [](const hy::entity_t& entity) { return entity.children_; })
           .value_or(std::vector<thh::handle_t>{}));
  }

  SUBCASE("move_down on first element moves to sibling if reachable") {
    interaction.element_ = 0;
    interaction.selected_ = thh::handle_t(1, 0);
    interaction.siblings_ =
      hy::siblings(interaction.selected_, entities, root_handles);

    demo::process_input(
      demo::input_e::move_down, entities, root_handles, interaction);

    CHECK(interaction.selected_ == thh::handle_t(2, 0));
    CHECK(interaction.element_ == 1);
    CHECK(
      interaction.siblings_
      == hy::siblings(interaction.selected_, entities, root_handles));
  }

  SUBCASE("move_down on second element moves to child if reachable") {
    interaction.element_ = 1;
    interaction.selected_ = thh::handle_t(2, 0);
    interaction.siblings_ =
      hy::siblings(interaction.selected_, entities, root_handles);

    demo::process_input(
      demo::input_e::move_down, entities, root_handles, interaction);

    CHECK(interaction.selected_ == thh::handle_t(5, 0));
    CHECK(interaction.element_ == 0);
    CHECK(
      interaction.siblings_
      == hy::siblings(interaction.selected_, entities, root_handles));
  }

  SUBCASE(
    "move_down on last element moves to next parent sibling if reachable") {
    interaction.element_ = 0;
    interaction.selected_ = thh::handle_t(10, 0);
    interaction.siblings_ =
      hy::siblings(interaction.selected_, entities, root_handles);

    demo::process_input(
      demo::input_e::move_down, entities, root_handles, interaction);

    CHECK(interaction.selected_ == thh::handle_t(11, 0));
    CHECK(interaction.element_ == 2);
    CHECK(
      interaction.siblings_
      == hy::siblings(interaction.selected_, entities, root_handles));
  }

  SUBCASE(
    "move_down on last element moves to root parent sibling if reachable") {
    interaction.element_ = 2;
    interaction.selected_ = thh::handle_t(11, 0);
    interaction.siblings_ =
      hy::siblings(interaction.selected_, entities, root_handles);

    demo::process_input(
      demo::input_e::move_down, entities, root_handles, interaction);

    CHECK(interaction.selected_ == thh::handle_t(7, 0));
    CHECK(interaction.element_ == 1);
    CHECK(interaction.siblings_ == root_handles);
  }

  SUBCASE("move_down on final element has no effect") {
    interaction.element_ = 0;
    interaction.selected_ = thh::handle_t(9, 0);
    interaction.siblings_ =
      hy::siblings(interaction.selected_, entities, root_handles);

    demo::process_input(
      demo::input_e::move_down, entities, root_handles, interaction);

    CHECK(interaction.selected_ == thh::handle_t(9, 0));
    CHECK(interaction.element_ == 0);
    CHECK(
      interaction.siblings_
      == hy::siblings(interaction.selected_, entities, root_handles));
  }

  SUBCASE("move_up on first element has no effect") {
    interaction.element_ = 0;
    interaction.selected_ = thh::handle_t(0, 0);
    interaction.siblings_ =
      hy::siblings(interaction.selected_, entities, root_handles);

    demo::process_input(
      demo::input_e::move_up, entities, root_handles, interaction);

    CHECK(interaction.selected_ == thh::handle_t(0, 0));
    CHECK(interaction.element_ == 0);
    CHECK(
      interaction.siblings_
      == hy::siblings(interaction.selected_, entities, root_handles));
  }

  SUBCASE("move_up on first child goes to parent") {
    interaction.element_ = 0;
    interaction.selected_ = thh::handle_t(1, 0);
    interaction.siblings_ =
      hy::siblings(interaction.selected_, entities, root_handles);

    demo::process_input(
      demo::input_e::move_up, entities, root_handles, interaction);

    CHECK(interaction.selected_ == thh::handle_t(0, 0));
    CHECK(interaction.element_ == 0);
    CHECK(
      interaction.siblings_
      == hy::siblings(interaction.selected_, entities, root_handles));
  }

  SUBCASE("move_up on later sibling goes to sibling") {
    interaction.element_ = 1;
    interaction.selected_ = thh::handle_t(6, 0);
    interaction.siblings_ =
      hy::siblings(interaction.selected_, entities, root_handles);

    demo::process_input(
      demo::input_e::move_up, entities, root_handles, interaction);

    CHECK(interaction.selected_ == thh::handle_t(5, 0));
    CHECK(interaction.element_ == 0);
    CHECK(
      interaction.siblings_
      == hy::siblings(interaction.selected_, entities, root_handles));
  }

  SUBCASE("move_up on expanded sibling goes to sibling child") {
    interaction.element_ = 2;
    interaction.selected_ = thh::handle_t(11, 0);
    interaction.siblings_ =
      hy::siblings(interaction.selected_, entities, root_handles);

    demo::process_input(
      demo::input_e::move_up, entities, root_handles, interaction);

    CHECK(interaction.selected_ == thh::handle_t(10, 0));
    CHECK(interaction.element_ == 0);
    CHECK(
      interaction.siblings_
      == hy::siblings(interaction.selected_, entities, root_handles));
  }

  SUBCASE("move_up on expanded sibling goes to most expanded sibling child") {
    interaction.element_ = 1;
    interaction.selected_ = thh::handle_t(7, 0);
    interaction.siblings_ =
      hy::siblings(interaction.selected_, entities, root_handles);

    demo::process_input(
      demo::input_e::move_up, entities, root_handles, interaction);

    CHECK(interaction.selected_ == thh::handle_t(11, 0));
    CHECK(interaction.element_ == 2);
    CHECK(
      interaction.siblings_
      == hy::siblings(interaction.selected_, entities, root_handles));
  }
}
