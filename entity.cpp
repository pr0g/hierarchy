#include "entity.hpp"

#include <deque>

namespace hy {
  bool try_move_right(
    interaction_t& interaction,
    const thh::container_t<hy::entity_t>& entities) {
    bool succeeded = false;
    if (!interaction.is_collapsed(interaction.selected_)) {
      entities.call(interaction.selected_, [&](const auto& entity) {
        if (!entity.children_.empty()) {
          interaction.selected_ = entity.children_.front();
          interaction.neighbors_ = entity.children_;
          interaction.element_ = 0;
          succeeded = true;
        }
      });
    }
    return succeeded;
  }

  bool try_move_left(
    interaction_t& interaction, const thh::container_t<hy::entity_t>& entities,
    const std::vector<thh::handle_t>& root_handles) {
    bool succeeded = false;
    entities.call(interaction.selected_, [&](const auto& entity) {
      entities.call(entity.parent_, [&](const auto& parent) {
        interaction.selected_ = entity.parent_;
        bool has_grandparent = false;
        entities.call(parent.parent_, [&](const auto& grandparent) {
          has_grandparent = true;
          interaction.neighbors_ = grandparent.children_;
        });
        if (!has_grandparent) {
          interaction.neighbors_ = root_handles;
        }
        interaction.element_ =
          std::find(
            interaction.neighbors_.begin(), interaction.neighbors_.end(),
            interaction.selected_)
          - interaction.neighbors_.begin();
        succeeded = true;
      });
    });
    return succeeded;
  }

  void move_up(interaction_t& interaction) {
    interaction.element_ =
      (interaction.element_ + interaction.neighbors_.size() - 1)
      % interaction.neighbors_.size();
    interaction.selected_ = interaction.neighbors_[interaction.element_];
  }

  void move_down(interaction_t& interaction) {
    interaction.element_ =
      (interaction.element_ + 1) % interaction.neighbors_.size();
    interaction.selected_ = interaction.neighbors_[interaction.element_];
  }

  void toggle_collapsed(interaction_t& interaction) {
    if (interaction.is_collapsed(interaction.selected_)) {
      interaction.collapsed_.erase(
        std::remove(
          interaction.collapsed_.begin(), interaction.collapsed_.end(),
          interaction.selected_),
        interaction.collapsed_.end());
    } else {
      interaction.collapsed_.push_back(interaction.selected_);
    }
  }

  void display_hierarchy(
    const thh::container_t<hy::entity_t>& entities,
    const interaction_t& interaction,
    const std::vector<thh::handle_t>& root_handles,
    const display_name_fn& display_name,
    const display_connection_fn& display_connection,
    const get_row_col_fn& get_row_col) {
    std::deque<thh::handle_t> entity_handle_stack;
    for (auto it = root_handles.rbegin(); it != root_handles.rend(); ++it) {
      entity_handle_stack.push_front(*it);
    }

    struct indent_tracker_t {
      int indent_ = 0;
      int count_ = 0;
    };

    std::deque<indent_tracker_t> indent_tracker;
    indent_tracker.push_front(
      indent_tracker_t{0, (int)entity_handle_stack.size()});

    const int indent_size = 4;
    while (!entity_handle_stack.empty()) {
      const auto curr_indent = indent_tracker.front().indent_;

      {
        auto& indent_ref = indent_tracker.front();
        indent_ref.count_--;
        if (indent_ref.count_ == 0) {
          indent_tracker.pop_front();
        }
      }

      auto entity_handle = entity_handle_stack.front();
      entity_handle_stack.pop_front();

      auto [row, col] = get_row_col();
      for (const auto ind : indent_tracker) {
        if (ind.count_ != 0 && ind.indent_ != curr_indent) {
          display_connection(row, ind.indent_);
        }
      }

      entities.call(entity_handle, [&, r = row, c = col](const auto& entity) {
        const auto selected = interaction.selected_ == entity_handle;
        display_name(
          r, c + curr_indent, selected,
          interaction.is_collapsed(entity_handle) && !entity.children_.empty(),
          entity.name_);

        const auto& children = entity.children_;
        if (!children.empty() && !interaction.is_collapsed(entity_handle)) {
          indent_tracker.push_front(
            indent_tracker_t{curr_indent + indent_size, (int)children.size()});
          for (auto it = children.rbegin(); it != children.rend(); ++it) {
            entity_handle_stack.push_front(*it);
          }
        }
      });
    }
  }
} // namespace hy

namespace demo {
  std::vector<thh::handle_t> create_sample_entities(
    thh::container_t<hy::entity_t>& entities) {
    using namespace std::string_literals;
    const int64_t handle_count = 11;
    std::vector<thh::handle_t> handles;
    handles.reserve(handle_count);
    for (int64_t i = 0; i < handle_count; i++) {
      auto handle = entities.add("entity_"s + std::to_string(i));
      handles.push_back(handle);
    }

    hy::add_children(handles[0], {handles[1], handles[2]}, entities);
    hy::add_children(handles[6], {handles[10]}, entities);
    hy::add_children(handles[7], {handles[3], handles[4]}, entities);
    hy::add_children(handles[2], {handles[5], handles[6]}, entities);
    hy::add_children(handles[8], {handles[9]}, entities);

    return {handles[0], handles[7],handles[8]};
  }
} // namespace demo
