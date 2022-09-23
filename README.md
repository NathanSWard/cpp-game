# **Nova**

### **Terminology & Overview**
- **app**: A runnable program containing logic.
- **registry**: An `entt::registry` with a few added featrues.
- **system**: A function that gets executed in the application.
- **resource**: *Globally unique* data accessible from systems.
  - There are two types: `Resource` and `Local`
  - `Resource`'s are shared between systems. E.g. if you modify a `Resource` from system `A` that change is visible by system `B`.
  - `Local`'s are unique to systems. E.g. if you modify a `Local` from system `A` that only affects the `Local` for that specific system. Other systems cannot view/modify other system's `Local`s.
- **world**: An aggregation of **resources** and the **registry**.
- **stage**: A collection of systems that can possibly be run in parallel.
- **scheduler**: The collection of stages that are run in order.
- **label**: An identifier used for ordering and lookup.
- **bundle**: A logical collection of components. It is **NOT** itself a component.

### **Labels**
- A `label` is used as an identifier for ordering systems and stages.
- They can be constructed from multiple types including:
  - Empty structs
  - String-like things
  - Systems

```cpp
struct MyLabel {};

auto system1 = [](nova::World&) {};
auto system2 = [](nova::World&) {};

app.add_system(nova::system(system1)
  // string-like things
  .label("a")
  .label(std::string{"string"})
  // empty structs
  .after<MyLabel>() // .after(MyLabel{}) <-- also valid syntax for empty structs
  // other systems
  .before(system2));
```

### **Stages**
- A stage is nothing other than a collection of systems.
- It can be visualized as
```cpp
struct Stage {
   std::string name;
   std::vector<System> systems;
};
```
- An application can have _any_ number of stages. These stages can (like systems) be ordered.
```cpp
// Adding a stage
auto app = nova::App{};

struct MyTagStage {};

// stages can be `tag` types
app.add_stage<MyTagStage>();

// or stages can be made from string like types
app.add_stage("my stage");

// stages can be ordered (just like systems)
app.add_stage(
    nova::stage("my other stage")
        .after("my stage")
        .after<nova::stages::Update>()
        .before<MyTagStage>());
```
- By default, nova has **5** stages that are run in order.
  - `First`
  - `PreUpdate`
  - `Update`
  - `PostUpdate`
  - `Last`
- If a system is added to the app without specifying the stage, it will be added to the `Update` stage.

### **Systems**
- A system is nothing other than a function which operates on a subset of the `World`.
- Systems can take any number of parameters, however, they are limited to their type.
- Notably, a system can be a normal function, a lambda, or any other invocable like object.
```cpp
// a normal function
auto my_system(nova::World& world) { /* ... */ }

// or a lambda
auto my_system = [](nova::World& world) {};
```

### **Startup/Teardown Systems**
- A `startup` system is one that runs **ONCE** at the _beginning_ of an app.
- A `teardown` system is one that runs **ONCE** at the _end_ of an app.
```cpp
auto my_startup_system() {}
auto my_teardown_system() {}

app.add_startup_system(my_startup_system);
app.add_teardown_system(my_teardown_system);
```

#### **System Parameters**
- `Local<T>`: a system specific resource.
- `Resource<T>`: a global resource shared between systems.
  - Must be initialized via `App::insert_resource` otherwise an exception will be thrown.
- `Option<Resource<T>>`:
    - If it is unknown if a resource exists, use this type. As it will _not_ throw an exception if the resource does not exist.
- `(const) Registry&`: a reference to the underlying `entt::registry`.
- `(const) Resources&`: a reference to all global resources.
- `(const) World&`: a reference to the applications' `World`.
- `View<With<...>, Without<...>>`: A simple view over all entities with a specific criteria of components.
- This is exactly the same as the type returned from `entt::registry::view<...>(...)`.

### **Adding & Ordering Systems**
- Systems can be added to an application via a simple `add_system()` call.
```cpp
auto my_system(nova::World& world) {}

auto app = nova::App{};
// be default, this is added to the `Update` stage.
app.add_system(my_system);
```
- Or they can be added to specific stages.
```cpp
app.add_system_to_stage<stages::PreUpdate>(my_system);
app.add_system_to_stage(my_other_system, "my custom stage");
```
- Ordering systems is very simple!
```cpp
auto system_a() -> void {}
auto system_b() -> void {}

// We are labeling the system 'a'
app.add_system(system(system_a).label("a"));

// We are ordering this systems *after* 'a'
// This means it will run *after* system 'a' completes.
app.add_system(system(system_b).after("a"));
```

### **SystemSet**
- A `system_set` is merely a way to assign similar labels/criteria to multiple systems.
```cpp
// instead of (note how both systems have the exact same labels/ordering)
app.add_system(nova::system(system1).label("a").after("b"))
   .add_system(nova::system(system2).label("a").after("b"));

// Use a system_set :)
app.add_system(nova::system_set()
                .with_system(system1)
                .with_system(system2)
                .label("a")
                .after("b"));
```

### A note on `const`-ness.
  - When using any of the following system parameters:
    - `Resource<T>`, `World`, `Registry`, `Resources`, or `View<...>`
  - It is important to mark the immutable uses of them as `const`.
  - As games are usually run in a multi-threaded environment, when two systems operate over mutable access to the same value, they **CANNOT** be run at the same time as this would cause a data race.
  - However, if you have two systems that operate on disjoint set of types, or share only **immutable** access to shared types, they **CAN** run at the same time.
```cpp
struct component { /* ... */ };

// immutable access to `component`
auto system_a(View<With<const component>>) {}
auto system_b(View<With<const component>>) {}

// mutable access to `component`
auto system_c(View<With<component>>) {}

/*
Since `system_a` and `system_b` have a CONST (immutable) view over `component` they can run at the same time.

Since `system_c` has NON CONST (mutable) access to `component` it cannot run at the same time as either `system_a` nor `system_b`.
*/
```

### **Bundles**
- TODO