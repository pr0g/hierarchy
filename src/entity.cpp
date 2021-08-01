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

  bool collapser_t::collapsed(const thh::handle_t handle) const {
    return std::find(collapsed_.begin(), collapsed_.end(), handle)
        != collapsed_.end();
  }

  bool collapser_t::expanded(const thh::handle_t handle) const {
    return !collapsed(handle);
  }

  void collapser_t::expand(const thh::handle_t entity_handle) {
    if (collapsed(entity_handle)) {
      collapsed_.erase(
        std::remove(collapsed_.begin(), collapsed_.end(), entity_handle),
        collapsed_.end());
    }
  }

  void collapser_t::collapse(
    const thh::handle_t entity_handle,
    const thh::container_t<hy::entity_t>& entities) {
    if (!collapsed(entity_handle) && has_children(entity_handle, entities)) {
      collapsed_.push_back(entity_handle);
    }
  }

  bool interaction_t::collapsed(const thh::handle_t entity_handle) const {
    return collapser_.collapsed(entity_handle);
  }

  void interaction_t::expand(const thh::handle_t entity_handle) {
    collapser_.expand(entity_handle);
  }

  void interaction_t::collapse(
    const thh::handle_t entity_handle,
    const thh::container_t<hy::entity_t>& entities) {
    collapser_.collapse(entity_handle, entities);
  }

  void interaction_t::select(
    const thh::handle_t entity_handle,
    const thh::container_t<entity_t>& entities,
    const std::vector<thh::handle_t>& root_handles) {
    if (entity_handle != thh::handle_t()) {
      selected_ = entity_handle;
      siblings_ = hy::siblings(entity_handle, entities, root_handles);
    }
  }

  void interaction_t::deselect() {
    selected_ = thh::handle_t();
    siblings_ = {};
  }

  int interaction_t::element() const {
    return std::find(siblings_.begin(), siblings_.end(), selected_)
         - siblings_.begin();
  }

  void interaction_t::move_up(
    const thh::container_t<hy::entity_t>& entities,
    const std::vector<thh::handle_t>& root_handles) {
    if (const auto location = element(); location != 0) {
      auto next_root = siblings_[location - 1];
      bool descended = false;
      while (has_children(next_root, entities) && !collapsed(next_root)) {
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
      if (collapsed(selected())) {
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

  int expanded_count(
    const thh::handle_t& entity_handle,
    const thh::container_t<hy::entity_t>& entities,
    const collapser_t& collapser) {
    std::vector<thh::handle_t> handles(1, entity_handle);
    int count = 1;
    while (!handles.empty()) {
      auto handle = handles.back();
      handles.pop_back();
      if (!collapser.collapsed(handle)) {
        entities.call(handle, [&](const auto& entity) {
          count += entity.children_.size();
          handles.insert(
            handles.end(), entity.children_.begin(), entity.children_.end());
        });
      }
    }
    return count;
  }

  struct indent_tracker_t {
    int indent_ = 0;
    int count_ = 0;
  };

  std::vector<flattened_handle> flatten_entity(
    const thh::handle_t entity_handle, const int indent,
    const thh::container_t<hy::entity_t>& entities,
    const collapser_t& collapser) {
    std::vector<flattened_handle> flattened;
    std::deque<indent_tracker_t> indent_tracker(1, {indent, 1});
    std::vector<thh::handle_t> handles(1, entity_handle);
    while (!handles.empty()) {
      const auto curr_indent = indent_tracker.front().indent_;

      if (!indent_tracker.empty()) {
        auto& indent_ref = indent_tracker.front();
        indent_ref.count_--;
        if (indent_ref.count_ == 0) {
          indent_tracker.pop_front();
        }
      }

      const auto handle = handles.back();
      flattened.push_back(flattened_handle{handle, curr_indent});
      handles.pop_back();
      entities.call(handle, [&](const auto& entity) {
        if (!entity.children_.empty() && !collapser.collapsed(handle)) {
          indent_tracker.push_front(
            indent_tracker_t{curr_indent + 1, (int)entity.children_.size()});
          handles.insert(
            handles.end(), entity.children_.rbegin(), entity.children_.rend());
        }
      });
    }
    return flattened;
  }

  std::vector<flattened_handle> flatten_entities(
    const thh::container_t<hy::entity_t>& entities,
    const collapser_t& collapser,
    const std::vector<thh::handle_t>& root_handles) {
    std::vector<flattened_handle> flattened;
    for (const auto root_handle : root_handles) {
      auto flattened_entity =
        flatten_entity(root_handle, 0, entities, collapser);
      flattened.insert(
        flattened.end(), flattened_entity.begin(), flattened_entity.end());
    }
    return flattened;
  }

  // go_to_entity (suppose all entities are loaded)
  // linear search to find entity in flattened
  // find_root (record offset)
  // record path back to root (expand all), record depth
  // doesn't need to find root, only last collapsed

  thh::handle_t collapsed_parent_handle(
    const thh::handle_t entity_handle,
    const thh::container_t<hy::entity_t>& entities, collapser_t& collapser) {
    auto search_handle = entity_handle;
    auto top_handle = thh::handle_t();
    while (search_handle != thh::handle_t()) {
      entities.call(
        search_handle,
        [&search_handle, &top_handle, &collapser](const hy::entity_t& entity) {
          if (collapser.collapsed(entity.parent_)) {
            collapser.expand(entity.parent_);
            top_handle = entity.parent_;
          }
          search_handle = entity.parent_;
        });
    }
    return top_handle;
  }

  std::pair<thh::handle_t, int> root_handle(
    const thh::handle_t entity_handle,
    const thh::container_t<hy::entity_t>& entities) {
    int depth = 0;
    auto search_handle = entity_handle;
    while (search_handle != thh::handle_t()) {
      bool found = entities
                     .call_return(
                       search_handle,
                       [&search_handle](const hy::entity_t& entity) {
                         if (entity.parent_ == thh::handle_t()) {
                           return true;
                         }
                         search_handle = entity.parent_;
                         return false;
                       })
                     .value();
      if (found) {
        break;
      } else {
        depth++;
      }
    }
    return {search_handle, depth};
  }

  int go_to_entity(
    const thh::handle_t entity_handle,
    const thh::container_t<hy::entity_t>& entities, collapser_t& collapser,
    std::vector<flattened_handle>& flattened_handles) {
    // might not be found if collapsed
    if (auto handle_it = std::find_if(
          flattened_handles.cbegin(), flattened_handles.cend(),
          [entity_handle](const flattened_handle& flattened_handle) {
            return flattened_handle.entity_handle_ == entity_handle;
          });
        handle_it != flattened_handles.cend()) {
      return handle_it - flattened_handles.cbegin();
    }
    // more complex if it's hidden
    // expand all parents that are collapsed, find top most, build from that
    auto collapsed_parent =
      collapsed_parent_handle(entity_handle, entities, collapser);
    int collapsed_parent_offset =
      std::find_if(
        flattened_handles.cbegin(), flattened_handles.cend(),
        [collapsed_parent](const flattened_handle& flattened_handle) {
          return flattened_handle.entity_handle_ == collapsed_parent;
        })
      - flattened_handles.cbegin();
    auto handles = hy::flatten_entity(
      collapsed_parent, flattened_handles[collapsed_parent_offset].indent_,
      entities, collapser);

    flattened_handles.insert(
      flattened_handles.begin() + collapsed_parent_offset + 1,
      handles.begin() + 1, handles.end());

    if (auto handle_it = std::find_if(
          flattened_handles.cbegin(), flattened_handles.cend(),
          [entity_handle](const flattened_handle& flattened_handle) {
            return flattened_handle.entity_handle_ == entity_handle;
          });
        handle_it != flattened_handles.cend()) {
      return handle_it - flattened_handles.cbegin();
    }

    return -1;
  }

  view_t::view_t(
    std::vector<flattened_handle> flattened_handles, const int offset,
    const int count)
    : flattened_handles_(std::move(flattened_handles)), offset_(offset),
      count_(count) {}

  void view_t::move_up() {
    selected_ = std::max(selected_ - 1, 0);
    if (selected_ - offset_ + 1 == 0) {
      offset_ = std::max(offset_ - 1, 0);
    }
  }

  void view_t::move_down() {
    const int min_offset = std::max((int)flattened_handles_.size() - count_, 0);
    selected_ = std::min(selected_ + 1, (int)flattened_handles_.size() - 1);
    if (selected_ - count_ - offset_ == 0) {
      offset_ = std::min(offset_ + 1, min_offset);
    }
  }

  void view_t::collapse(
    const thh::container_t<hy::entity_t>& entities, collapser_t& collapser) {
    const auto entity_handle = flattened_handles_[selected_].entity_handle_;
    int count = hy::expanded_count(entity_handle, entities, collapser);
    collapser.collapse(entity_handle, entities);
    flattened_handles_.erase(
      flattened_handles_.begin() + selected_ + 1,
      flattened_handles_.begin() + selected_ + count);
  }

  void view_t::expand(
    const thh::container_t<hy::entity_t>& entities, collapser_t& collapser) {
    const auto entity_handle = flattened_handles_[selected_].entity_handle_;
    if (collapser.collapsed(entity_handle)) {
      collapser.expand(entity_handle);
      auto handles = hy::flatten_entity(
        entity_handle, flattened_handles_[selected_].indent_, entities,
        collapser);
      flattened_handles_.insert(
        flattened_handles_.begin() + selected_ + 1, handles.begin() + 1,
        handles.end());
    }
  }

  void view_t::goto_recorded_handle(
    const thh::container_t<hy::entity_t>& entities, collapser_t& collapser) {
    if (recorded_handle_ != thh::handle_t()) {
      selected_ = hy::go_to_entity(
        recorded_handle_, entities, collapser, flattened_handles_);
      offset_ = selected_;
    }
  }

  void view_t::record_handle() {
    recorded_handle_ = flattened_handles_[selected_].entity_handle_;
  }

  void view_t::add_child(
    thh::container_t<hy::entity_t>& entities, collapser_t& collapser) {
    if (!collapser.collapsed(flattened_handles_[selected_].entity_handle_)) {
      auto next_handle = entities.add();
      entities.call(next_handle, [next_handle](auto& entity) {
        entity.name_ = std::string("entity_") + std::to_string(next_handle.id_);
      });
      hy::add_children(
        flattened_handles_[selected_].entity_handle_, {next_handle}, entities);
      const auto child_count = hy::expanded_count(
        flattened_handles_[selected_].entity_handle_, entities, collapser);
      flattened_handles_.insert(
        flattened_handles_.begin()
          + std::min(
            selected_ + child_count - 1, (int)flattened_handles_.size()),
        {next_handle, flattened_handles_[selected_].indent_ + 1});
    }
  }

  void view_t::add_sibling(
    thh::container_t<hy::entity_t>& entities, collapser_t& collapser,
    std::vector<thh::handle_t>& root_handles) {
    auto current_handle = flattened_handles_[selected_].entity_handle_;
    auto next_handle = entities.add();
    entities.call(next_handle, [next_handle](auto& entity) {
      entity.name_ = std::string("entity_") + std::to_string(next_handle.id_);
    });
    const auto siblings = hy::siblings(current_handle, entities, root_handles);

    int sibling_offset = 0;
    int siblings_left = 0;
    if (auto sibling_it =
          std::find(siblings.begin(), siblings.end(), current_handle);
        sibling_it != siblings.end()) {
      sibling_offset = sibling_it - siblings.begin();
      siblings_left = siblings.size() - sibling_offset;
    }

    int child_count = 0;
    for (int i = sibling_offset; i < siblings.size(); ++i) {
      child_count += hy::expanded_count(siblings[i], entities, collapser) - 1;
    }

    flattened_handles_.insert(
      flattened_handles_.begin() + selected_ + child_count + siblings_left,
      {next_handle, flattened_handles_[selected_].indent_});

    entities.call(current_handle, [&](hy::entity_t& entity) {
      auto parent_handle = entities
                             .call_return(
                               entity.parent_,
                               [&](hy::entity_t& parent_entity) {
                                 parent_entity.children_.push_back(next_handle);
                                 return entity.parent_;
                               })
                             .value_or(thh::handle_t());
      if (parent_handle != thh::handle_t()) {
        entities.call(next_handle, [parent_handle](hy::entity_t& entity) {
          entity.parent_ = parent_handle;
        });
      } else {
        root_handles.push_back(next_handle);
      }
    });
  }

  void display_scrollable_hierarchy(
    const thh::container_t<hy::entity_t>& entities,
    const std::vector<thh::handle_t>& root_handles, const view_t& view,
    const collapser_t& collapser, const display_ops_t& display_ops) {
    int min_indent = std::numeric_limits<int>::max();
    thh::handle_t min_indent_handle;

    std::vector<std::pair<int, int>> connections;
    const int total_handles = view.flattened_handles().size();
    const int min_handles =
      std::min(total_handles - view.offset(), view.count());
    // find another matching indent before a lower indent is found
    std::vector<bool> ends;
    // search 'upwards' first (reverse iterators)
    for (int indent_index = 0; indent_index < min_handles; ++indent_index) {
      const int row_index =
        std::min(indent_index + view.offset(), total_handles);
      if (view.flattened_handles()[row_index].indent_ < min_indent) {
        min_indent = view.flattened_handles()[row_index].indent_;
        min_indent_handle = view.flattened_handles()[row_index].entity_handle_;
      }
      const int prev_indent_index = indent_index - 1;
      if (auto rfound = std::find_if(
            view.flattened_handles().rbegin() + (total_handles - 1)
              - (view.offset() + prev_indent_index),
            view.flattened_handles().rend(),
            [&view, row_index](const auto& flattened_handle) {
              return view.flattened_handles()[row_index].indent_
                  == flattened_handle.indent_;
            });
          rfound != view.flattened_handles().rend()) {
        auto ffound = (rfound + 1).base();
        auto dist = ffound - view.flattened_handles().begin();
        if (dist < view.offset()) {
          if (std::all_of(
                view.flattened_handles().rbegin() + (total_handles - 1)
                  - (view.offset() + prev_indent_index),
                rfound, [&view, row_index](const auto& flattened_handle) {
                  return flattened_handle.indent_
                      >= view.flattened_handles()[row_index].indent_;
                })) {
            int range = rfound
                      - (view.flattened_handles().rbegin() + (total_handles - 1)
                         - (view.offset() + prev_indent_index));
            for (int i = 0; i < range; ++i) {
              int is = prev_indent_index - i;
              if (is < 0) {
                break;
              }
              connections.push_back(
                {view.flattened_handles()[row_index].indent_, is});
            }
          }
        }
      }
      const int next_indent_index = indent_index + 1;
      const int next_row_index =
        std::min(view.offset() + next_indent_index, total_handles);
      if (auto found = std::find_if(
            view.flattened_handles().begin() + next_row_index,
            view.flattened_handles().end(),
            [&view, row_index](const auto& flattened_handle) {
              return view.flattened_handles()[row_index].indent_
                  == flattened_handle.indent_;
            });
          found != view.flattened_handles().end()) {
        if (std::all_of(
              view.flattened_handles().begin() + next_row_index, found,
              [&view, row_index](const auto& flattened_handle) {
                return flattened_handle.indent_
                    >= view.flattened_handles()[row_index].indent_;
              })) {
          const int range =
            found - (view.flattened_handles().begin() + next_row_index);
          for (int i = 0; i < range; i++) {
            int is = next_indent_index + i;
            int min = std::min(
              view.count(),
              (int)view.flattened_handles().size() - view.offset());
            if (is >= min) {
              break;
            }
            connections.push_back(
              {view.flattened_handles()[row_index].indent_, is});
          }
          ends.push_back(false);
        } else {
          ends.push_back(true);
        }
      } else {
        ends.push_back(true);
      }
    }

    // draw all lines for entities that are offscreen
    for (int i = min_indent - 1; i >= 0; --i) {
      bool draw =
        entities
          .call_return(
            min_indent_handle,
            [&](const hy::entity_t& entity) {
              // update min indent as we walk backwards right to left
              min_indent_handle = entity.parent_;
              return entities
                .call_return(
                  entity.parent_,
                  [&](const hy::entity_t& parent) {
                    if (parent.parent_ == thh::handle_t()) {
                      if (auto it = std::find(
                            root_handles.begin(), root_handles.end(),
                            entity.parent_);
                          it != root_handles.end()) {
                        auto position = it - root_handles.begin();
                        return position != (root_handles.size() - 1);
                      }
                      return false;
                    }
                    return entities
                      .call_return(
                        parent.parent_,
                        [&](const hy::entity_t& grandparent) {
                          if (auto it = std::find(
                                grandparent.children_.begin(),
                                grandparent.children_.end(), entity.parent_);
                              it != grandparent.children_.end()) {
                            auto position = it - grandparent.children_.begin();
                            return position
                                != (grandparent.children_.size() - 1);
                          }
                          return false;
                        })
                      .value_or(false);
                  })
                .value_or(false);
            })
          .value_or(false);
      if (draw) {
        for (int r = 0; r < view.count(); r++) {
          display_ops.draw_at_fn(i * 4, r, "\xE2\x94\x82");
        }
      }
    }

    assert(ends.size() == std::min(min_handles, view.count()));

    for (const auto& connection : connections) {
      display_ops.draw_at_fn(
        connection.first * 4, connection.second, "\xE2\x94\x82");
    }

    const int count = std::min(
      (int)view.flattened_handles().size(), view.offset() + view.count());
    for (int handle_index = view.offset(); handle_index < count;
         ++handle_index) {
      const auto& flattened_handle = view.flattened_handles()[handle_index];

      display_ops.draw_at_fn(
        flattened_handle.indent_ * 4, handle_index - view.offset(),
        ends[handle_index - view.offset()]
          ? "\xE2\x94\x94\xE2\x94\x80\xE2\x94\x80 "
          : "\xE2\x94\x9C\xE2\x94\x80\xE2\x94\x80 ");

      if (handle_index == view.selected()) {
        display_ops.set_invert_fn(true);
      }
      if (collapser.collapsed(flattened_handle.entity_handle_)) {
        display_ops.set_bold_fn(true);
      }
      entities.call(flattened_handle.entity_handle_, [&](const auto& entity) {
        display_ops.draw_fn(entity.name_);
      });
      display_ops.set_invert_fn(false);
      display_ops.set_bold_fn(false);
    }
  }

  void display_hierarchy(
    const thh::container_t<hy::entity_t>& entities,
    const interaction_t& interaction,
    const std::vector<thh::handle_t>& root_handles, const display_fn& display,
    const scope_exit_fn& scope_exit,
    const display_connection_fn& display_connection) {
    std::deque<thh::handle_t> entity_handle_stack;
    for (auto it = root_handles.begin(); it != root_handles.end(); ++it) {
      entity_handle_stack.push_front(*it);
    }

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

      const auto entity_handle = entity_handle_stack.back();
      entity_handle_stack.pop_back();

      entities.call(entity_handle, [&](const auto& entity) {
        const auto& children = entity.children_;

        display_info_t display_info;
        display_info.level = level;
        display_info.indent = curr_indent;
        display_info.entity_handle = entity_handle;
        display_info.selected = interaction.selected() == entity_handle;
        display_info.collapsed =
          interaction.collapsed(entity_handle) && !children.empty();
        display_info.has_children = !children.empty();
        display_info.name = entity.name_;
        display_info.last = last_element;

        display(display_info);

        if (!children.empty() && !interaction.collapsed(entity_handle)) {
          indent_tracker.push_front(
            indent_tracker_t{curr_indent + 1, (int)children.size()});
          for (auto it = children.rbegin(); it != children.rend(); ++it) {
            entity_handle_stack.push_back(*it);
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
      const auto handle = entities.add();
      entities.call(handle, [handle](auto& entity) {
        entity.name_ = std::string("entity_") + std::to_string(handle.id_);
      });
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

  std::vector<thh::handle_t> create_bench_entities(
    thh::container_t<hy::entity_t>& entities, const int root_count,
    const int handle_count) {
    using namespace std::string_literals;
    std::vector<thh::handle_t> roots;
    for (int64_t r = 0; r < root_count; r++) {
      std::vector<thh::handle_t> handles;
      handles.reserve(handle_count);
      for (int64_t i = 0; i < handle_count; i++) {
        const auto handle = entities.add();
        entities.call(handle, [handle](auto& entity) {
          entity.name_ = std::string("entity_") + std::to_string(handle.id_);
        });
        handles.push_back(handle);
      }

      for (int64_t i = 0; i < handle_count - 1; ++i) {
        hy::add_children(handles[i], {handles[i + 1]}, entities);
      }

      roots.push_back(handles[0]);
    }

    return roots;
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
        interaction.expand(interaction.selected());
        break;
      case input_e::collapse:
        interaction.collapse(interaction.selected(), entities);
        break;
      case input_e::add_child: {
        if (!interaction.collapsed(interaction.selected())) {
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
