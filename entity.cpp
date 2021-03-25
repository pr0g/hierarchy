#include "entity.hpp"

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
