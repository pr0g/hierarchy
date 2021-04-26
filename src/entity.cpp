#include "hierarchy/entity.hpp"

#include <deque>
#include <numeric>

namespace hy {
  bool has_children(
    const thh::handle_t handle,
    const thh::container_t<hy::entity_t>& entities) {
    return entities
      .call_return(
        handle, [&](const auto& entity) { return !entity.children_.empty(); })
      .value_or(false);
  }

  std::vector<thh::handle_t> siblings(
    const thh::handle_t entity_handle,
    const thh::container_t<entity_t>& entities,
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

  void add_children(
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

  void interaction_t::move_up(
    const thh::container_t<hy::entity_t>& entities,
    const std::vector<thh::handle_t>& root_handles) {
    if (const auto location = element(); location != 0) {
      auto next_root = siblings_[location - 1];
      bool descended = false;
      while (has_children(next_root, entities) && !is_collapsed(next_root)) {
        entities.call(next_root, [&](const auto& entity) {
          next_root = entity.children_.back();
          descended = true;
        });
      }
      if (descended) {
        entities.call(next_root, [&](const auto& entity) {
          entities.call(entity.parent_, [&](const auto& parent) {
            select(parent.children_.back(), entities, root_handles);
          });
        });
      } else {
        selected_ = siblings_[element() - 1];
      }
    } else {
      entities.call(selected(), [&](const auto& entity) {
        select(entity.parent_, entities, root_handles);
      });
    }
  }

  void interaction_t::move_down(
    const thh::container_t<hy::entity_t>& entities,
    const std::vector<thh::handle_t>& root_handles) {
    const auto next_sibling = [&]() {
      if (const int next_element = element() + 1;
          next_element == siblings_.size()) {
        entities.call(selected(), [&](const auto& entity) {
          thh::handle_t ancestor = entity.parent_;
          std::vector<thh::handle_t> ancestor_siblings = siblings_;
          while (ancestor != thh::handle_t()) {
            const bool next = entities
                                .call_return(
                                  ancestor,
                                  [&](const auto& parent) {
                                    const auto ancestor_siblings =
                                      entities
                                        .call_return(
                                          parent.parent_,
                                          [](const auto& grandparent) {
                                            return grandparent.children_;
                                          })
                                        .value_or(root_handles);
                                    const auto element =
                                      std::find(
                                        ancestor_siblings.begin(),
                                        ancestor_siblings.end(), ancestor)
                                      - ancestor_siblings.begin() + 1;
                                    if (element == ancestor_siblings.size()) {
                                      ancestor = parent.parent_;
                                    } else {
                                      siblings_ = ancestor_siblings;
                                      selected_ = siblings_[element];
                                      return true;
                                    }
                                    return false;
                                  })
                                .value_or(false);
            if (next) {
              break;
            }
          }
        });
      } else {
        selected_ = siblings_[next_element];
      }
    };

    if (has_children(selected(), entities)) {
      if (is_collapsed(selected())) {
        next_sibling();
      } else {
        entities.call(selected(), [&](const auto& entity) {
          selected_ = entity.children_.front();
          siblings_ = entity.children_;
        });
      }
    } else {
      next_sibling();
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
        display_info.selected = interaction.selected() == entity_handle;
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
        interaction.move_up(entities, root_handles);
        break;
      case input_e::move_down:
        interaction.move_down(entities, root_handles);
        break;
      case input_e::expand:
        interaction.expand_selected();
        break;
      case input_e::collapse:
        interaction.collapse_selected(entities);
        break;
      case input_e::add_child: {
        if (!interaction.is_collapsed(interaction.selected())) {
          auto next_handle = entities.add();
          entities.call(next_handle, [next_handle](auto& entity) {
            entity.name_ =
              std::string("entity_") + std::to_string(next_handle.id_);
          });
          hy::add_children(interaction.selected(), {next_handle}, entities);
        }
      } break;
      default:
        break;
    }
  }
} // namespace demo
