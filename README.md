# backwater

## Game Play

### Overview



## Game Data

### Data Entities

* Galaxy - top level entity that manages all game data
    * contains `users` & `stories`
* Story - meta-entity which facilitates procedurally generated game data (the objects in the game and the history of how they got there)
    * contains `sectors`, `scenarios`, and `story` data (e.g. cutures, history, relationships)
* Scenario - meta-entity which represents a mini-game within a `story` that `users` can join (as a `player`) 
    * `scenarios` have a lifetime which begins when the first `player` joins and ends when the last `player` dies or retires
    * contains `scenario` data (e.g. setup, players, and relationships)
* Sector - contains simulation data

### Galaxy Setup

1. Initialize and run a `galaxy`.  A `galaxy` is a single realtime object that creates `stories` & `scenarios`, tracks `users` and active `scenarios`, and controls the running of `sectors`.
2. The `galaxy` will generate `stories` on demand which will provide `scenarios` that `users` can join.  A `story` will require the creation of `sectors` which contain all the information related to the spacial simulation.
3. The galaxy will initialize and run `sectors` involved in active `scenarios`.  Active `scenarios` are those which have been joined by a `user`.

### Story Setup

1. Select a `story` template
2. If the `story` template requires a parent `story`, select a parent `story` at random.
3. Create the `sectors` requiried for the `story`
    * generate features
    * generate feature related objects
    * generate humanity arrival, events, and diaspera
    * generate humanity related objects (e.g. colonies, outposts, wreckage, and ruins)
4. Create `scenarios` related to history of humanity in this `story`
    * selected from `scenario` templates
    * generate `scenario` related objects for `sector`

## Architecture

### Components

* client
    * unity exe
    * client lib
    * zone server
* server
    * proxy server (2+/region)
    * session server (1)
    * galaxy server (1)
    * content server (1+/region)
    * console server (1+/region)
    * sector server (1+/region)
    * view server  (1+/region)

### Features

* proxy protocol (tcp)
* view protocol (udp)
* view console
    * spacial sim using box2d
    * arbitrary coordinates
    * view specific client data
* distributed zone server
    * zone <-> zone
    * zone <-> view
* regionalized data (msg routing & data ids)
* 

## Project

### Proof-of-Concept

- [x] simulation: UDP peer-to-peer NAT traversal
- [ ] simulation: zone server : multi-zone simulation using physics engine
- [ ] simulation: view simulation : relative coords and ids, object visibility (cloud, lighting, emit)
- [ ] rendering: Object Shadow Casting w/ soft & hard edges
- [ ] rendering: Cloud lighting
- [ ] rendering: Partial Terrain Rendering (large asteroid edges)
