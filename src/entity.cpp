#include "hierarchy/entity.hpp"

#include <deque>
#include <numeric>

namespace hy {
  namespace {
    bool has_children(
      const thh::handle_t handle,
      const thh::container_t<hy::entity_t>& entities) {
      return entities
        .call_return(
          handle, [&](const auto& entity) { return !entity.children_.empty(); })
        .value_or(false);
    }
  } // namespace

  void move_up(
    interaction_t& interaction, const thh::container_t<hy::entity_t>& entities,
    const std::vector<thh::handle_t>& root_handles) {

    const auto location = interaction.element_;
    if (location != 0) {
      auto next_root = interaction.neighbors_[interaction.element_ - 1];
      bool descended = false;
      while (true) {
        const bool children = has_children(next_root, entities);
        const bool expanded = !interaction.is_collapsed(next_root);
        if (children && expanded) {
          entities.call(next_root, [&](const auto& entity) {
            next_root = entity.children_.back();
            descended = true;
          });
        } else {
          break;
        }
      }

      if (descended) {
        entities.call(next_root, [&](const auto& entity) {
          entities.call(entity.parent_, [&](const auto& parent) {
            interaction.neighbors_ = parent.children_;
            interaction.element_ = interaction.neighbors_.size() - 1;
            interaction.selected_ =
              interaction.neighbors_[interaction.element_];
          });
        });
      } else {
        interaction.element_--;
        interaction.selected_ = interaction.neighbors_[interaction.element_];
      }
    } else {
      entities.call(interaction.selected_, [&](const auto& entity) {
        entities.call(entity.parent_, [&](const auto& parent) {
          bool has_grandparent = false;
          entities.call(parent.parent_, [&](const auto& grandparent) {
            has_grandparent = true;
            interaction.neighbors_ = grandparent.children_;
          });
          if (!has_grandparent) {
            interaction.neighbors_ = root_handles;
          }
          interaction.element_ = std::find(
                                   interaction.neighbors_.begin(),
                                   interaction.neighbors_.end(), entity.parent_)
                               - interaction.neighbors_.begin();
          interaction.selected_ = interaction.neighbors_[interaction.element_];
        });
      });
    }
  }

  void move_down(
    interaction_t& interaction, const thh::container_t<hy::entity_t>& entities,
    const std::vector<thh::handle_t>& root_handles) {
    auto next_neighbor = [&]() {
      const int next_element = interaction.element_ + 1;
      if (next_element == interaction.neighbors_.size()) {
        entities.call(interaction.selected_, [&](const auto& entity) {
          bool next = false;
          thh::handle_t ancestor = entity.parent_;
          std::vector<thh::handle_t> ancestor_neighbors =
            interaction.neighbors_;
          while (!next && ancestor != thh::handle_t()) {
            entities.call(ancestor, [&](const auto& parent) {
              bool has_grandparent = false;
              entities.call(parent.parent_, [&](const auto& grandparent) {
                has_grandparent = true;
                ancestor_neighbors = grandparent.children_;
              });
              if (!has_grandparent) {
                ancestor_neighbors = root_handles;
              }
              auto element = std::find(
                               ancestor_neighbors.begin(),
                               ancestor_neighbors.end(), ancestor)
                           - ancestor_neighbors.begin() + 1;
              if (element == ancestor_neighbors.size()) {
                ancestor = parent.parent_;
              } else {
                interaction.element_ = element;
                interaction.neighbors_ = ancestor_neighbors;
                interaction.selected_ =
                  interaction.neighbors_[interaction.element_];
                next = true;
              }
            });
          }
        });
      } else {
        interaction.element_ = next_element;
        interaction.selected_ = interaction.neighbors_[interaction.element_];
      }
    };

    if (has_children(interaction.selected_, entities)) {
      if (interaction.is_collapsed(interaction.selected_)) {
        next_neighbor();
      } else {
        entities.call(interaction.selected_, [&](const auto& entity) {
          interaction.selected_ = entity.children_.front();
          interaction.neighbors_ = entity.children_;
          interaction.element_ = 0;
        });
      }
    } else {
      next_neighbor();
    }
  }

  void collapse(
    interaction_t& interaction,
    const thh::container_t<hy::entity_t>& entities) {
    if (
      !interaction.is_collapsed(interaction.selected_)
      && has_children(interaction.selected_, entities)) {
      interaction.collapsed_.push_back(interaction.selected_);
    }
  }

  void expand(interaction_t& interaction) {
    if (interaction.is_collapsed(interaction.selected_)) {
      interaction.collapsed_.erase(
        std::remove(
          interaction.collapsed_.begin(), interaction.collapsed_.end(),
          interaction.selected_),
        interaction.collapsed_.end());
    }
  }

  void display_hierarchy(
    const thh::container_t<hy::entity_t>& entities,
    const interaction_t& interaction,
    const std::vector<thh::handle_t>& root_handles, const display_fn& display,
    const scope_exit_fn& scope_exit,
    const display_connection_fn& display_connection) {
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

    int level = 0; // the level (row) in the hierarchy
    int last_indent = 0; // most recent indent (col)
    while (!entity_handle_stack.empty()) {
      const auto curr_indent = indent_tracker.front().indent_;

      int last = last_indent;
      while (curr_indent < last) {
        scope_exit();
        last--;
      }

      const bool last_element = [&] {
        auto& indent_ref = indent_tracker.front();
        indent_ref.count_--;
        if (indent_ref.count_ == 0) {
          indent_tracker.pop_front();
          return true;
        }
        return false;
      }();

      for (const auto ind : indent_tracker) {
        if (ind.count_ != 0 && ind.indent_ != curr_indent) {
          display_connection(level, ind.indent_);
        }
      }

      const auto entity_handle = entity_handle_stack.front();
      entity_handle_stack.pop_front();

      entities.call(entity_handle, [&](const auto& entity) {
        const auto& children = entity.children_;

        display_info_t display_info;
        display_info.level = level;
        display_info.indent = curr_indent;
        display_info.entity_handle = entity_handle;
        display_info.selected = interaction.selected_ == entity_handle;
        display_info.collapsed =
          interaction.is_collapsed(entity_handle) && !children.empty();
        display_info.has_children = !children.empty();
        display_info.name = entity.name_;
        display_info.last = last_element;

        display(display_info);

        if (!children.empty() && !interaction.is_collapsed(entity_handle)) {
          indent_tracker.push_front(
            indent_tracker_t{curr_indent + 1, (int)children.size()});
          for (auto it = children.rbegin(); it != children.rend(); ++it) {
            entity_handle_stack.push_front(*it);
          }
        }
      });
      level++;
      last_indent = curr_indent;
    }

    while (last_indent > 0) {
      scope_exit();
      last_indent--;
    }
  }
} // namespace hy

namespace demo {
  std::vector<thh::handle_t> create_sample_entities(
    thh::container_t<hy::entity_t>& entities) {
    using namespace std::string_literals;
    const int64_t handle_count = 12;
    std::vector<thh::handle_t> handles;
    handles.reserve(handle_count);
    for (int64_t i = 0; i < handle_count; i++) {
      auto handle = entities.add("entity_"s + std::to_string(i));
      handles.push_back(handle);
    }

    hy::add_children(handles[0], {handles[1], handles[2]}, entities);
    hy::add_children(handles[6], {handles[10]}, entities);
    hy::add_children(handles[7], {handles[3], handles[4]}, entities);
    hy::add_children(
      handles[2], {handles[5], handles[6], handles[11]}, entities);
    hy::add_children(handles[8], {handles[9]}, entities);

    return {handles[0], handles[7], handles[8]};
  }

  void process_input(
    const input_e input, thh::container_t<hy::entity_t>& entities,
    const std::vector<thh::handle_t>& root_handles,
    hy::interaction_t& interaction) {
    switch (input) {
      case input_e::move_up:
        hy::move_up(interaction, entities, root_handles);
        break;
      case input_e::move_down:
        hy::move_down(interaction, entities, root_handles);
        break;
      case input_e::expand:
        hy::expand(interaction);
        break;
      case input_e::collapse:
        hy::collapse(interaction, entities);
        break;
      case input_e::add_child: {
        auto next_handle = entities.add();
        entities.call(next_handle, [next_handle](auto& entity) {
          entity.name_ =
            std::string("entity_") + std::to_string(next_handle.id_);
        });
        hy::add_children(
          interaction.neighbors_[interaction.element_], {next_handle},
          entities);
      } break;
      default:
        break;
    }
  }
} // namespace demo
