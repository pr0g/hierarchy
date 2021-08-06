# todo

## display

- test rendering with Dear ImGui
- ~~test rendering with Qt~~
- add `end_display` function to be called in `indent == 0` block
  - use to call tree pop function

## operation

- ~~add ability to add a neighbor/peer/sibling entity~~
- ~~add ability to delete an entity~~
- ~~don't allow adding children to collapsed entities~~

## other

- ~~add `#define` for building demo app~~

## tests

- add last element crash
- lookup + view.offset being out of range
- test lower level individual functions
  - flatten_entity/entities
  - expanded_count
  - add_children
  - add_child
  - add_sibling
  - siblings
  - entity_and_descendants
  - collapsed_parent_handle
  - root_handle
  - go_to_entity
  - expand/collapse
  - remove
- add test to clear 'collapsed' handles
- update 'collapsed' handles to use a hash table (unordered_map)
- investigate generating coverage info again

## bench

- test expand/collapse performance with different counts
- test rendering performance with different entity counts
