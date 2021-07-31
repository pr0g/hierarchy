#pragma once

#include <thh_handles/thh_handles.hpp>

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

  void add_children(
    thh::handle_t entity_handle,
    const std::vector<thh::handle_t>& child_handles,
    thh::container_t<entity_t>& entities);

  std::vector<thh::handle_t> siblings(
    thh::handle_t entity_handle, const thh::container_t<entity_t>& entities,
    const std::vector<thh::handle_t>& root_handles);

  bool has_children(
    thh::handle_t handle, const thh::container_t<hy::entity_t>& entities);

  struct collapser_t {
    void expand(thh::handle_t entity_handle);
    void collapse(
      thh::handle_t entity_handle,
      const thh::container_t<hy::entity_t>& entities);
    bool expanded(thh::handle_t handle) const;
    bool collapsed(thh::handle_t handle) const;

  private:
    std::vector<thh::handle_t> collapsed_;
  };

  int expanded_count(
    const thh::handle_t& entity_handle,
    const thh::container_t<hy::entity_t>& entities,
    const collapser_t& collapser);

  struct flattened_handle {
    thh::handle_t entity_handle_;
    int32_t indent_;
  };

  int go_to_entity(
    thh::handle_t entity_handle, const thh::container_t<hy::entity_t>& entities,
    collapser_t& collapser, std::vector<flattened_handle>& flattened_handles);

  std::vector<flattened_handle> flatten_entity(
    thh::handle_t entity_handle, int indent,
    const thh::container_t<hy::entity_t>& entities,
    const collapser_t& collapser);

  std::vector<flattened_handle> flatten_entities(
    const thh::container_t<hy::entity_t>& entities,
    const collapser_t& collapser,
    const std::vector<thh::handle_t>& root_handles);

  // view into the collection of entities
  struct view_t {
    explicit view_t(
      std::vector<flattened_handle> flattened_handles, int offset, int count)
      : flattened_handles_(std::move(flattened_handles)), offset_(offset),
        count_(count) {}

    void move_up() {
      selected_ = std::max(selected_ - 1, 0);
      if (selected_ - offset_ + 1 == 0) {
        offset_ = std::max(offset_ - 1, 0);
      }
    }

    void move_down() {
      const int min_offset =
        std::max((int)flattened_handles_.size() - count_, 0);
      selected_ = std::min(selected_ + 1, (int)flattened_handles_.size() - 1);
      if (selected_ - count_ - offset_ == 0) {
        offset_ = std::min(offset_ + 1, min_offset);
      }
    }

    void collapse(
      const thh::container_t<hy::entity_t>& entities, collapser_t& collapser) {
      const auto entity_handle = flattened_handles_[selected_].entity_handle_;
      int count = hy::expanded_count(entity_handle, entities, collapser);
      collapser.collapse(entity_handle, entities);
      flattened_handles_.erase(
        flattened_handles_.begin() + selected_ + 1,
        flattened_handles_.begin() + selected_ + count);
    }

    void expand(
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

    void goto_handle(
      const thh::container_t<hy::entity_t>& entities, collapser_t& collapser) {
      if (goto_handle_ != thh::handle_t()) {
        selected_ = hy::go_to_entity(
          goto_handle_, entities, collapser, flattened_handles_);
        offset_ = selected_;
      }
    }

    void record_handle() {
      goto_handle_ = flattened_handles_[selected_].entity_handle_;
    }

    void add_child(
      thh::container_t<hy::entity_t>& entities, collapser_t& collapser) {
      if (!collapser.collapsed(flattened_handles_[selected_].entity_handle_)) {
        auto next_handle = entities.add();
        entities.call(next_handle, [next_handle](auto& entity) {
          entity.name_ =
            std::string("entity_") + std::to_string(next_handle.id_);
        });
        hy::add_children(
          flattened_handles_[selected_].entity_handle_, {next_handle},
          entities);
        const auto child_count = hy::expanded_count(
          flattened_handles_[selected_].entity_handle_, entities, collapser);
        flattened_handles_.insert(
          flattened_handles_.begin() + selected_ + child_count - 1,
          {next_handle, flattened_handles_[selected_].indent_ + 1});
      }
    }

    void add_sibling(
      thh::container_t<hy::entity_t>& entities, collapser_t& collapser,
      std::vector<thh::handle_t>& root_handles) {
      auto current_handle = flattened_handles_[selected_].entity_handle_;
      auto next_handle = entities.add();
      entities.call(next_handle, [next_handle](auto& entity) {
        entity.name_ = std::string("entity_") + std::to_string(next_handle.id_);
      });
      const auto siblings =
        hy::siblings(current_handle, entities, root_handles);

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
        auto parent_handle =
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
    }

    const std::vector<flattened_handle>& flattened_handles() const {
      return flattened_handles_;
    }

    int offset() const { return offset_; }
    int count() const { return count_; }
    int selected() const { return selected_; }

  private:
    std::vector<flattened_handle> flattened_handles_;
    int offset_ = 0;
    int selected_ = 0;
    int count_ = 20;
    thh::handle_t goto_handle_;
  };

  std::pair<thh::handle_t, int> root_handle(
    thh::handle_t entity_handle,
    const thh::container_t<hy::entity_t>& entities);

  thh::handle_t collapsed_parent_handle(
    thh::handle_t entity_handle, const thh::container_t<hy::entity_t>& entities,
    collapser_t& collapser);

  enum class input_e {
    move_up,
    move_down,
    expand,
    collapse,
    add_child,
    add_sibling,
    record,
    go_to
  };

  struct display_ops_t {
    std::function<void(bool)> set_bold_fn;
    std::function<void(bool)> set_invert_fn;
    std::function<void(int, int, std::string_view)> draw_at_fn;
    std::function<void(std::string_view)> draw_fn;
  };

  void display_scrollable_hierarchy(
    const thh::container_t<hy::entity_t>& entities,
    const std::vector<thh::handle_t>& root_handles, const view_t& view,
    const collapser_t& collapser, const display_ops_t& display_ops);

  //////////////////////////////////////////////////////////////////////////////

  struct interaction_t {
    bool collapsed(thh::handle_t handle) const;

    void select(
      thh::handle_t entity_handle, const thh::container_t<entity_t>& entities,
      const std::vector<thh::handle_t>& root_handles);

    void deselect();
    void expand(thh::handle_t entity_handle);
    void collapse(
      thh::handle_t entity_handle,
      const thh::container_t<hy::entity_t>& entities);
    thh::handle_t selected() const { return selected_; }
    int element() const;
    void move_down(
      const thh::container_t<hy::entity_t>& entities,
      const std::vector<thh::handle_t>& root_handles);
    void move_up(
      const thh::container_t<hy::entity_t>& entities,
      const std::vector<thh::handle_t>& root_handles);

    const std::vector<thh::handle_t>& siblings() const { return siblings_; }

  private:
    thh::handle_t selected_;
    std::vector<thh::handle_t> siblings_;
    collapser_t collapser_;
  };

  struct model_t {
    thh::container_t<entity_t> entities_;
    std::vector<thh::handle_t> root_;
    interaction_t interaction_;
  };

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

  std::vector<thh::handle_t> create_bench_entities(
    thh::container_t<hy::entity_t>& entities, const int root_count,
    const int handle_count);

  enum class input_e { move_up, move_down, expand, collapse, add_child };

  void process_input(
    const input_e input, thh::container_t<hy::entity_t>& entities,
    const std::vector<thh::handle_t>& root_handles,
    hy::interaction_t& interaction);
} // namespace demo
