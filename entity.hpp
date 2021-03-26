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

  struct interaction_t {
    int element_ = 0;
    std::vector<thh::handle_t> neighbors_;

    thh::handle_t selected_;
    std::vector<thh::handle_t> collapsed_;

    bool is_collapsed(thh::handle_t handle) const {
      return std::find(collapsed_.begin(), collapsed_.end(), handle)
          != collapsed_.end();
    }
  };

  bool try_move_right(
    interaction_t& interaction, thh::container_t<hy::entity_t>& entities);
  bool try_move_left(
    interaction_t& interaction, thh::container_t<hy::entity_t>& entities,
    const std::vector<thh::handle_t>& root_handles);
  void move_up(interaction_t& interaction);
  void move_down(interaction_t& interaction);
  void toggle_collapsed(interaction_t& interaction);

  using display_name_fn =
    std::function<void(int, int, bool, bool, const std::string& name)>;
  using display_connection_fn = std::function<void(int, int)>;
  using get_row_col_fn = std::function<std::pair<int, int>()>;

  void display_hierarchy(
    const thh::container_t<hy::entity_t>& entities,
    const interaction_t& interaction,
    const std::vector<thh::handle_t>& root_handles,
    const display_name_fn& display_name,
    const display_connection_fn& display_connection,
    const get_row_col_fn& get_row_col);
} // namespace hy

namespace demo {
  std::vector<thh::handle_t> create_sample_entities(
    thh::container_t<hy::entity_t>& entities);
}
