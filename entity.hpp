#pragma once

#include "thh_handles/thh_handles.hpp"

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

    bool is_collapsed(thh::handle_t handle) {
      return std::find(collapsed_.begin(), collapsed_.end(), handle)
          != collapsed_.end();
    }
  };
} // namespace hy

namespace demo {
  std::vector<thh::handle_t> create_sample_entities(
    thh::container_t<hy::entity_t>& entities);
}
