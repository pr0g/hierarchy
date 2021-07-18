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

  int expanded_count(
    const thh::handle_t& entity_handle,
    const thh::container_t<hy::entity_t>& entities,
    const collapser_t& collapser);

  // view into the collection of entities
  struct view_t {
    int offset_ = 0;
    int selected_ = 0;
    int count_ = 20;
  };

  struct handle_flattened {
    thh::handle_t entity_handle_;
    int32_t indent_;
  };

  std::pair<thh::handle_t, int> root_handle(
    thh::handle_t entity_handle,
    const thh::container_t<hy::entity_t>& entities);

  thh::handle_t collapsed_parent_handle(
    thh::handle_t entity_handle, const thh::container_t<hy::entity_t>& entities,
    collapser_t& collapser);

  int go_to_entity(
    thh::handle_t entity_handle, const thh::container_t<hy::entity_t>& entities,
    collapser_t& collapser, std::vector<handle_flattened>& flattened);

  std::vector<handle_flattened> build_hierarchy_single(
    thh::handle_t entity_handle, int starting_indent,
    const thh::container_t<hy::entity_t>& entities,
    const collapser_t& collapser);

  std::vector<handle_flattened> build_vector(
    const thh::container_t<hy::entity_t>& entities,
    const collapser_t& collapser,
    const std::vector<thh::handle_t>& root_handles);

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
    const thh::container_t<hy::entity_t>& entities, const view_t& view,
    const interaction_t& interaction,
    const std::vector<thh::handle_t>& root_handles, const display_fn& display,
    const scope_exit_fn& scope_exit,
    const display_connection_fn& display_connection);
} // namespace hy

namespace demo {
  std::vector<thh::handle_t> create_sample_entities(
    thh::container_t<hy::entity_t>& entities);

  std::vector<thh::handle_t> create_bench_entities(
    thh::container_t<hy::entity_t>& entities);

  enum class input_e { move_up, move_down, expand, collapse, add_child };

  void process_input(
    const input_e input, thh::container_t<hy::entity_t>& entities,
    const std::vector<thh::handle_t>& root_handles,
    hy::interaction_t& interaction);
} // namespace demo
