#include "entity.hpp"

namespace hy {
  bool try_move_right(
    interaction_t& interaction, thh::container_t<hy::entity_t>& entities) {
    if (!interaction.is_collapsed(interaction.selected_)) {
      if (hy::entity_t* entity = entities.resolve(interaction.selected_)) {
        if (!entity->children_.empty()) {
          interaction.selected_ = entity->children_.front();
          interaction.neighbors_ = entity->children_;
          interaction.element_ = 0;
          return true;
        }
      }
    }
    return false;
  }

  bool try_move_left(
    interaction_t& interaction, thh::container_t<hy::entity_t>& entities,
    const std::vector<thh::handle_t>& root_handles) {
    if (hy::entity_t* entity = entities.resolve(interaction.selected_)) {
      if (hy::entity_t* parent = entities.resolve(entity->parent_)) {
        interaction.selected_ = entity->parent_;
        if (hy::entity_t* grandparent = entities.resolve(parent->parent_)) {
          interaction.neighbors_ = grandparent->children_;
        } else {
          interaction.neighbors_ = root_handles;
        }
        interaction.element_ =
          std::find(
            interaction.neighbors_.begin(), interaction.neighbors_.end(),
            interaction.selected_)
          - interaction.neighbors_.begin();
        return true;
      }
    }
    return false;
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
} // namespace hy

namespace demo {
  std::vector<thh::handle_t> create_sample_entities(
    thh::container_t<hy::entity_t>& entities) {
    auto handle_a = entities.add("entity_a");
    auto handle_b = entities.add("entity_b");
    auto handle_c = entities.add("entity_c");
    auto handle_d = entities.add("entity_d");
    auto handle_e = entities.add("entity_e");
    auto handle_f = entities.add("entity_f");
    auto handle_g = entities.add("entity_g");
    auto handle_h = entities.add("entity_h");
    auto handle_i = entities.add("entity_i");
    auto handle_j = entities.add("entity_j");
    auto handle_k = entities.add("entity_k");

    auto* entity_a = entities.resolve(handle_a);
    auto* entity_b = entities.resolve(handle_b);
    auto* entity_c = entities.resolve(handle_c);
    auto* entity_d = entities.resolve(handle_d);
    auto* entity_e = entities.resolve(handle_e);
    auto* entity_f = entities.resolve(handle_f);
    auto* entity_g = entities.resolve(handle_g);
    auto* entity_h = entities.resolve(handle_h);
    auto* entity_i = entities.resolve(handle_i);
    auto* entity_j = entities.resolve(handle_j);
    auto* entity_k = entities.resolve(handle_k);

    entity_a->children_.push_back(handle_b);
    entity_a->children_.push_back(handle_c);
    entity_b->parent_ = handle_a;
    entity_c->parent_ = handle_a;

    entity_g->children_.push_back(handle_k);
    entity_k->parent_ = handle_g;

    entity_h->children_.push_back(handle_d);
    entity_h->children_.push_back(handle_e);
    entity_d->parent_ = handle_h;
    entity_e->parent_ = handle_h;

    entity_c->children_.push_back(handle_f);
    entity_c->children_.push_back(handle_g);
    entity_f->parent_ = handle_c;
    entity_g->parent_ = handle_c;

    entity_i->children_.push_back(handle_j);
    entity_j->parent_ = handle_i;

    return {handle_a, handle_h, handle_i};
  }
} // namespace demo
