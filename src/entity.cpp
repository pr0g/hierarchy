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

  std::vector<flattened_handle_t> flatten_entity(
    const thh::handle_t entity_handle, const int indent,
    const thh::container_t<hy::entity_t>& entities,
    const collapser_t& collapser) {
    std::vector<flattened_handle_t> flattened;
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
      flattened.push_back(flattened_handle_t{handle, curr_indent});
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

  std::vector<thh::handle_t> entity_and_descendants(
    thh::handle_t entity_handle,
    const thh::container_t<hy::entity_t>& entities) {
    std::vector<thh::handle_t> all_handles;
    std::vector<thh::handle_t> handles(1, entity_handle);
    while (!handles.empty()) {
      const auto handle = handles.back();
      all_handles.push_back(handle);
      handles.pop_back();
      entities.call(handle, [&](const auto& entity) {
        if (!entity.children_.empty()) {
          handles.insert(
            handles.end(), entity.children_.rbegin(), entity.children_.rend());
        }
      });
    }
    return all_handles;
  }

  std::vector<flattened_handle_t> flatten_entities(
    const thh::container_t<hy::entity_t>& entities,
    const collapser_t& collapser,
    const std::vector<thh::handle_t>& root_handles) {
    std::vector<flattened_handle_t> flattened;
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

  std::optional<int> go_to_entity(
    const thh::handle_t entity_handle,
    const thh::container_t<hy::entity_t>& entities, collapser_t& collapser,
    std::vector<flattened_handle_t>& flattened_handles) {
    // might not be found if collapsed
    if (auto handle_it = std::find_if(
          flattened_handles.cbegin(), flattened_handles.cend(),
          [entity_handle](const flattened_handle_t& flattened_handle) {
            return flattened_handle.entity_handle_ == entity_handle;
          });
        handle_it != flattened_handles.cend()) {
      return (int)(handle_it - flattened_handles.cbegin());
    }
    // more complex if it's hidden
    // expand all parents that are collapsed, find top most, build from that
    auto collapsed_parent =
      collapsed_parent_handle(entity_handle, entities, collapser);
    int collapsed_parent_offset =
      std::find_if(
        flattened_handles.cbegin(), flattened_handles.cend(),
        [collapsed_parent](const flattened_handle_t& flattened_handle) {
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
          [entity_handle](const flattened_handle_t& flattened_handle) {
            return flattened_handle.entity_handle_ == entity_handle;
          });
        handle_it != flattened_handles.cend()) {
      return (int)(handle_it - flattened_handles.cbegin());
    }

    return {};
  }

  view_t::view_t(
    std::vector<flattened_handle_t> flattened_handles, const int offset,
    const int count)
    : flattened_handles_(std::move(flattened_handles)), offset_(offset),
      count_(count) {}

  thh::handle_t view_t::selected_handle() const {
    if (!selected_index().has_value()) {
      return thh::handle_t();
    }
    return flattened_handles_[selected_.value()].entity_handle_;
  }

  std::optional<int> view_t::selected_index() const {
    if (flattened_handles_.empty()) {
      return {};
    }
    return selected_;
  }

  std::optional<int> view_t::selected_indent() const {
    if (!selected_index().has_value()) {
      return {};
    }
    return flattened_handles_[selected_.value()].indent_;
  }

  void view_t::move_up() {
    if (!selected_index().has_value()) {
      return;
    }
    selected_ = std::max(*selected_ - 1, 0);
    if (*selected_ - offset_ + 1 == 0) {
      offset_ = std::max(offset_ - 1, 0);
    }
  }

  void view_t::move_down() {
    if (!selected_index().has_value()) {
      return;
    }
    const int min_offset = std::max((int)flattened_handles_.size() - count_, 0);
    selected_ = std::min(*selected_ + 1, (int)flattened_handles_.size() - 1);
    if (*selected_ - count_ - offset_ == 0) {
      offset_ = std::min(offset_ + 1, min_offset);
    }
  }

  void view_t::collapse(
    const thh::container_t<hy::entity_t>& entities, collapser_t& collapser) {
    if (const auto entity_handle = selected_handle();
        entity_handle != thh::handle_t()) {
      const int expanded_count =
        hy::expanded_count(entity_handle, entities, collapser);
      collapser.collapse(entity_handle, entities);
      flattened_handles_.erase(
        flattened_handles_.begin() + *selected_ + 1,
        flattened_handles_.begin() + *selected_ + expanded_count);
    }
  }

  void view_t::expand(
    const thh::container_t<hy::entity_t>& entities, collapser_t& collapser) {
    if (const auto entity_handle = selected_handle();
        entity_handle != thh::handle_t()) {
      if (collapser.collapsed(entity_handle)) {
        collapser.expand(entity_handle);
        auto handles = hy::flatten_entity(
          entity_handle, *selected_indent(), entities, collapser);
        flattened_handles_.insert(
          flattened_handles_.begin() + *selected_ + 1, handles.begin() + 1,
          handles.end());
      }
    }
  }

  void view_t::goto_recorded_handle(
    const thh::container_t<hy::entity_t>& entities, collapser_t& collapser) {
    if (recorded_handle_ != thh::handle_t()) {
      if (auto selected = hy::go_to_entity(
            recorded_handle_, entities, collapser, flattened_handles_);
          selected.has_value()) {
        selected_ = selected;
        offset_ = *selected_;
      }
    }
  }

  void view_t::record_handle() { recorded_handle_ = selected_handle(); }

  void view_t::clear_recorded_handle() { recorded_handle_ = thh::handle_t(); }

  std::optional<flattened_handle_position_t> view_t::add_child(
    thh::container_t<hy::entity_t>& entities, collapser_t& collapser) {
    const auto selected = selected_handle();
    if (selected == thh::handle_t()) {
      return {};
    }
    if (!collapser.collapsed(selected)) {
      auto next_handle = entities.add();
      entities.call(next_handle, [next_handle](auto& entity) {
        entity.name_ = std::string("entity_") + std::to_string(next_handle.id_);
      });
      hy::add_children(selected, {next_handle}, entities);
      const auto child_count =
        hy::expanded_count(selected, entities, collapser);
      const auto inserted = flattened_handles_.insert(
        flattened_handles_.begin()
          + std::min(
            *selected_index() + child_count - 1,
            (int)flattened_handles_.size()),
        {next_handle, *selected_indent() + 1});

      return flattened_handle_position_t{
        *inserted, int32_t(inserted - flattened_handles_.begin())};
    }
    return {};
  }

  flattened_handle_position_t view_t::add_sibling(
    thh::container_t<hy::entity_t>& entities, collapser_t& collapser,
    std::vector<thh::handle_t>& root_handles) {
    const auto next_handle = entities.add();
    entities.call(next_handle, [next_handle](auto& entity) {
      entity.name_ = std::string("entity_") + std::to_string(next_handle.id_);
    });

    auto handle = selected_handle();
    if (handle == thh::handle_t()) {
      selected_ = 0;
      handle = next_handle;
    }

    const auto siblings =
      hy::siblings(selected_handle(), entities, root_handles);

    int sibling_offset = 0;
    int siblings_left = 0;
    if (const auto sibling_it =
          std::find(siblings.cbegin(), siblings.cend(), selected_handle());
        sibling_it != siblings.cend()) {
      sibling_offset = sibling_it - siblings.begin();
      siblings_left = siblings.size() - sibling_offset;
    }

    int child_count = 0;
    for (int sibling_index = sibling_offset; sibling_index < siblings.size();
         ++sibling_index) {
      child_count +=
        hy::expanded_count(siblings[sibling_index], entities, collapser) - 1;
    }

    const auto inserted = flattened_handles_.insert(
      flattened_handles_.begin() + selected_index().value_or(0) + child_count
        + siblings_left,
      {next_handle, selected_indent().value_or(0)});

    entities.call(handle, [&](hy::entity_t& entity) {
      const auto parent_handle =
        entities
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

    return flattened_handle_position_t{
      *inserted, int32_t(inserted - flattened_handles_.begin())};
  }

  void view_t::remove(
    thh::container_t<hy::entity_t>& entities, collapser_t& collapser,
    std::vector<thh::handle_t>& root_handles) {
    if (const auto handle = selected_handle(); handle != thh::handle_t()) {
      if (entities
            .call_return(
              handle,
              [&](const entity_t& entity) {
                return entity.parent_ == thh::handle_t();
              })
            .value_or(false)) {
        if (auto root =
              std::find(root_handles.cbegin(), root_handles.cend(), handle);
            root != root_handles.cend()) {
          root_handles.erase(root);
        }
      }

      const auto entity_and_descendants =
        hy::entity_and_descendants(handle, entities);
      const auto expanded_count =
        hy::expanded_count(handle, entities, collapser);

      entities.call(handle, [&](hy::entity_t& entity) {
        entities.call(entity.parent_, [&](hy::entity_t& parent) {
          parent.children_.erase(std::remove_if(
            parent.children_.begin(), parent.children_.end(),
            [handle](const thh::handle_t& h) { return handle == h; }));
        });
      });

      for (const auto& h : entity_and_descendants) {
        [[maybe_unused]] const bool removed = entities.remove(h);
        assert(removed);
      }

      flattened_handles_.erase(
        flattened_handles_.begin() + *selected_index(),
        flattened_handles_.begin() + *selected_index() + expanded_count);

      selected_ = std::min((int)flattened_handles_.size() - 1, *selected_);
      offset_ =
        std::min(std::max((int)flattened_handles_.size() - 1, 0), offset_);
    }

    // todo - need to also remove from collapsed
  }

  void display_scrollable_hierarchy(
    const thh::container_t<hy::entity_t>& entities,
    const std::vector<thh::handle_t>& root_handles, const view_t& view,
    const collapser_t& collapser, const display_ops_t& display_ops) {
    thh::handle_t min_indent_handle;
    int min_indent = std::numeric_limits<int>::max();

    std::vector<std::pair<int, int>> connections;
    const int total_handles = view.flattened_handles().size();
    const int min_visible_handles =
      std::min(total_handles - view.offset(), view.count());
    // find another matching indent before a lower indent is found
    std::vector<bool> ends;
    ends.reserve(std::min(min_visible_handles, view.count()));
    for (int row_index = 0; row_index < min_visible_handles; ++row_index) {
      const int handle_index =
        std::min(row_index + view.offset(), total_handles);
      if (view.flattened_handles()[handle_index].indent_ < min_indent) {
        min_indent = view.flattened_handles()[handle_index].indent_;
        min_indent_handle =
          view.flattened_handles()[handle_index].entity_handle_;
      }
      // search 'upwards' first (reverse iterators)
      const int prev_row_index = row_index - 1;
      const int prev_handle_index =
        (total_handles - 1) - (view.offset() + prev_row_index);
      if (auto found = std::find_if(
            view.flattened_handles().rbegin() + prev_handle_index,
            view.flattened_handles().rend(),
            [&view, handle_index](const auto& flattened_handle) {
              return view.flattened_handles()[handle_index].indent_
                  == flattened_handle.indent_;
            });
          found != view.flattened_handles().rend()) {
        const auto dist = (found + 1).base() - view.flattened_handles().begin();
        if (dist < view.offset()) {
          if (std::all_of(
                view.flattened_handles().rbegin() + prev_handle_index, found,
                [&view, handle_index](const auto& flattened_handle) {
                  return flattened_handle.indent_
                      >= view.flattened_handles()[handle_index].indent_;
                })) {
            for (int i = prev_row_index; i >= 0; i--) {
              connections.push_back(
                {view.flattened_handles()[handle_index].indent_, i});
            }
          }
        }
      }
      const int next_row_index = row_index + 1;
      const int next_handle_index =
        std::min(view.offset() + next_row_index, total_handles);
      if (auto found = std::find_if(
            view.flattened_handles().begin() + next_handle_index,
            view.flattened_handles().end(),
            [&view, handle_index](const auto& flattened_handle) {
              return view.flattened_handles()[handle_index].indent_
                  == flattened_handle.indent_;
            });
          found != view.flattened_handles().end()) {
        if (std::all_of(
              view.flattened_handles().begin() + next_handle_index, found,
              [&view, handle_index](const auto& flattened_handle) {
                return flattened_handle.indent_
                    >= view.flattened_handles()[handle_index].indent_;
              })) {
          const int range =
            found - (view.flattened_handles().begin() + next_handle_index);
          for (int i = next_row_index;
               i < std::min(next_row_index + range, view.count()); i++) {
            connections.push_back(
              {view.flattened_handles()[handle_index].indent_, i});
          }
          ends.push_back(false);
        } else {
          ends.push_back(true);
        }
      } else {
        ends.push_back(true);
      }
    }

    // can not be set if handles are empty
    if (min_indent != std::numeric_limits<int>::max()) {
      // draw all lines for entities that are offscreen
      for (int indent_index = min_indent - 1; indent_index >= 0;
           --indent_index) {
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
                              auto position =
                                it - grandparent.children_.begin();
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
          for (int row = 0; row < view.count(); row++) {
            display_ops.draw_at_fn_(
              indent_index * display_ops.indent_width_, row,
              display_ops.connection_);
          }
        }
      }
    }

    assert(ends.size() == std::min(min_visible_handles, view.count()));

    for (const auto& connection : connections) {
      display_ops.draw_at_fn_(
        connection.first * display_ops.indent_width_, connection.second,
        display_ops.connection_);
    }

    const int count = std::min(
      (int)view.flattened_handles().size(), view.offset() + view.count());
    for (int handle_index = view.offset(); handle_index < count;
         ++handle_index) {
      const auto& flattened_handle = view.flattened_handles()[handle_index];

      display_ops.draw_at_fn_(
        flattened_handle.indent_ * display_ops.indent_width_,
        handle_index - view.offset(),
        ends[handle_index - view.offset()] ? display_ops.end_
                                           : display_ops.mid_);

      if (handle_index == view.selected_index()) {
        display_ops.set_invert_fn_(true);
      }
      if (collapser.collapsed(flattened_handle.entity_handle_)) {
        display_ops.set_bold_fn_(true);
      }
      entities.call(flattened_handle.entity_handle_, [&](const auto& entity) {
        display_ops.draw_fn_(entity.name_);
      });
      display_ops.set_invert_fn_(false);
      display_ops.set_bold_fn_(false);
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
} // namespace demo
