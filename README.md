# backwater

## Game Play

### Overview



## Game Data

### Data Entities

* Galaxy - top level entity that manages all game data
    * contains `users` & `stories`
* Story - meta-entity which facilitates procedurally generated game data (the objects in the game and the history of how they got there)
    * contains the `sectors` and `scenarios` which support the story
    * story generation involves:
        * development of an initial spacial layout (`sector`)
        * arrival of historical humanity (history of how humanity got here)
        * development of humanity over long periods of time: mythologies, cultures, civic structures
        * calamities, devistation, and other events which place the development of humanity in the current time frame (`sector` and `scenarios`)
* Scenario - meta-entity which represents a mini-game within a `story` that `users` can join (as a `player`) 
    * A `scenario` is a set of interconnected ways that players can interact with the `story` (they involve the same region of space and the same history to interact with)
    * `scenarios` have a lifetime which begins when the first `player` joins and ends when the last `player` dies or retires
    * contains `scenario` data (e.g. player setup, the players, and the player relationships to non-player-entities)
* Sector - contains simulation data

### Galaxy Setup

1. Initialize and run a `galaxy`.  A `galaxy` is a single realtime object that creates `stories` & `scenarios`, tracks `users` and active `scenarios`, and controls the simulation of `sectors`.
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

## Simulation Data

### Data Entities

* Deep Object - background object which is visible and has orientation, but no other effect; used for navigation
    * e.g. Star Cluster, Nebula
* Feature - region visible at a distance; can move toward and interact with it
    * e.g. Dust Cloud, Asteroid Cluster, Star, Tempest
* Object - Asteroid, Ship, Debris, Cloud
* ObjectNode
    * ref point for model or `ObjectModule`
* ObjectNodeLink
    * link between nodes
* ObjectModule
    * people containing thing
* ObjectModuleLink
    * link between modules

## Architecture

### Components

* client
    * unity exe
    * client lib
    * zone server lib
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

## Project

### Proof-of-Concept

- [x] simulation: UDP peer-to-peer NAT traversal
- [ ] simulation: zone server : multi-zone simulation using physics engine
- [ ] simulation: view simulation : relative coords and ids, object visibility (cloud, lighting, emit)
- [ ] rendering: Object Shadow Casting w/ soft & hard edges
- [ ] rendering: Cloud lighting
- [ ] rendering: Partial Terrain Rendering (large asteroid edges)
- [ ] data: inventory & thing system, movement of things in the world
- [ ] data: knowledge of things: limits on description & user operations

### Notes

* Simulation
    * Overview
        * 
    * Problems
        1. how to distribute simulation accross multiple hosts (scale)
            * make zone-servers communicate peer-to-peer, pass off objects
            * adjacent zones overlap simulation for seemless hand-off
        2. how to synchronize clocks
            * simulation inputs
            * zone-to-zone timing
            * maybe sync every 15m with centralized server, use 16bit timestamps for 60000 milli accuracy
        3. how to send real-time temporary state (e.g. inputs or effects)
            * maybe repeat every frame for duration
            * maybe repeat changes until confirmation
            * maybe use TCP w/ timestamp
        4. how to generate unique ids for objects created by zone-servers
            * maybe leave unassigned until it is saved to sector server
                * naw, need view servers to be able to reference object ids to sector server
            * maybe sector provides reserved object ids to zone-servers
        5. how to run at dynamic fps for cpu variations
            * sleep cycles too long: true fps lower than target fps
            * sleep cycles too short: 
            * inconsistant frame times
    * Entities: zone-server, zone
        * zone-server can contain multiple zones, handle communication with servers of adjacent zones.
    * Interfaces
        * ISectorToZoneServer
            * updateSectorInfo              // zone server needs sector info before it gets zone assignment
            * removeSectorInfo
            * updateZone                    // zone assignment is made, or told about peer zone server
            * removeZone
            * updateObject
            * removeObject
            * updateView
            * removeView
        * IZoneToSectorServer
            * addObject                     // sector generates ID
            * updateObject                  // sector saves object data
            * removeObject
        * IZoneToZoneServer
            * updateObject
            * removeObject
        * IZoneToViewServer
            * updateObject
            * removeObject
        * IViewToZoneServer
            * watch
            * controlObject

* Client Initialization
    * Problems
        * Disconnect > Reconnect - what happens?
            * Does session get reused, or new session?
            * Can client reinitialize without restarting server's session state?
        * For Client
            * Get user info, player instances, event history
            * 
        * For Galaxy Server
        * For Content Server
        * For Console Server
        * For Sector Server
    * Steps
        * Connect to proxy
        * Authenticate
        * Hello to Galaxy
            1. Prepare server access -> view,
            1. Get user data
            2. Get view session
        * Hello to View
            1. Send client's udp endpoint to server
            2. Get server's udp endpoint + key