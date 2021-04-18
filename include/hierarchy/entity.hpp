#pragma once

#include "thh_handles/thh_handles.hpp"

#include <functional>
#include <string>
#include <vector>

namespace hy {
  struct entity_t {
    entity_t() = default;
    explicit entity_t(std::string name) : name_(std::move(name)) {}
    std::string name_;
    std::vector<thh::handle_t> children_;
    thh::handle_t parent_;
  };

  inline void add_children(
    const thh::handle_t entity_handle,
    const std::vector<thh::handle_t>& child_handles,
    thh::container_t<entity_t>& entities) {
    entities.call(entity_handle, [&child_handles](auto& entity) {
      entity.children_.insert(
        entity.children_.end(), child_handles.begin(), child_handles.end());
    });
    std::for_each(
      child_handles.begin(), child_handles.end(),
      [&entities, entity_handle](const auto child_handle) {
        entities.call(child_handle, [entity_handle](auto& entity) {
          entity.parent_ = entity_handle;
        });
      });
  }

  inline std::vector<thh::handle_t> siblings(
    thh::handle_t entity_handle, const thh::container_t<entity_t>& entities,
    const std::vector<thh::handle_t>& root_handles) {
    return entities
      .call_return(
        entity_handle,
        [&](const entity_t& entity) {
          return entities
            .call_return(
              entity.parent_,
              [](const entity_t& parent) { return parent.children_; })
            .value_or(root_handles);
        })
      .value_or(std::vector<thh::handle_t>{});
  }

  struct interaction_t {
    int element_ = 0;
    std::vector<thh::handle_t> siblings_;

    thh::handle_t selected_;
    std::vector<thh::handle_t> collapsed_;

    bool is_collapsed(const thh::handle_t handle) const {
      return std::find(collapsed_.begin(), collapsed_.end(), handle)
          != collapsed_.end();
    }
  };

  void move_up(
    interaction_t& interaction, const thh::container_t<hy::entity_t>& entities,
    const std::vector<thh::handle_t>& root_handles);
  void move_down(
    interaction_t& interaction, const thh::container_t<hy::entity_t>& entities,
    const std::vector<thh::handle_t>& root_handles);

  void expand(interaction_t& interaction);
  void collapse(
    interaction_t& interaction, const thh::container_t<hy::entity_t>& entities);

  // info required when displaying each element
  struct display_info_t {
    int level;
    int indent;
    thh::handle_t entity_handle;
    bool selected;
    bool collapsed;
    bool has_children;
    std::string name;
    bool last;
  };

  using display_fn = std::function<void(const display_info_t&)>;
  using scope_exit_fn = std::function<void()>;
  // level, indent
  using display_connection_fn = std::function<void(int, int)>;

  void display_hierarchy(
    const thh::container_t<hy::entity_t>& entities,
    const interaction_t& interaction,
    const std::vector<thh::handle_t>& root_handles, const display_fn& display,
    const scope_exit_fn& scope_exit,
    const display_connection_fn& display_connection);
} // namespace hy

namespace demo {
  std::vector<thh::handle_t> create_sample_entities(
    thh::container_t<hy::entity_t>& entities);

  enum class input_e { move_up, move_down, expand, collapse, add_child };

  void process_input(
    const input_e input, thh::container_t<hy::entity_t>& entities,
    const std::vector<thh::handle_t>& root_handles,
    hy::interaction_t& interaction);
} // namespace demo
