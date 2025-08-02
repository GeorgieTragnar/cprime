# Complex Inheritance Pattern Translations

## Overview

This document provides systematic translations of complex C++ inheritance patterns to CPrime's compositional polymorphism system, demonstrating that every inheritance pattern can be represented more clearly and safely in CPrime.

## Translation Methodology

### Core Translation Rules

1. **State Unification**: All inherited state goes into a single compositional data structure
2. **Capability Exposure**: Virtual methods become access rights to functional classes
3. **Explicit Delegation**: Method overrides become explicit delegation chains
4. **Optional Components**: Derived class features become optional state components

### Translation Algorithm

```
For any C++ inheritance pattern:

1. Identify all classes in hierarchy: [Base, Derived1, Derived2, ...]
2. Extract all state: base_state + derived_states...
3. Extract all virtual methods: method_set
4. Create unified data structure with optional derived components
5. Create access rights exposing appropriate state to each capability
6. Create functional classes implementing method_set with explicit delegation
7. Use traits for interface contracts
```

## Pattern Category 1: Simple Inheritance Chains

### C++ Linear Inheritance

```cpp
class Vehicle {
protected:
    std::string model;
    int year;
public:
    virtual void start() { std::cout << "Vehicle starting\n"; }
    virtual void stop() { std::cout << "Vehicle stopping\n"; }
    virtual std::string getInfo() = 0;
    virtual ~Vehicle() = default;
};

class Car : public Vehicle {
protected:
    int doors;
    std::string fuel_type;
public:
    void start() override { 
        std::cout << "Car engine starting\n"; 
        Vehicle::start();
    }
    
    std::string getInfo() override {
        return "Car: " + model + " (" + std::to_string(year) + ")";
    }
    
    virtual void honk() { std::cout << "Beep beep!\n"; }
};

class SportsCar : public Car {
private:
    int horsepower;
    bool turbo;
public:
    void start() override {
        std::cout << "Sports car roaring to life!\n";
        Car::start();
    }
    
    std::string getInfo() override {
        return Car::getInfo() + " - " + std::to_string(horsepower) + "HP";
    }
    
    void enableTurbo() { turbo = true; }
};
```

### CPrime Translation

```cpp
class VehicleData {
    // All possible state unified
    model: String,
    year: i32,
    car_doors: Option<i32>,
    fuel_type: Option<String>,
    sports_horsepower: Option<i32>,
    sports_turbo: Option<bool>,
    
    // Capability exposure based on what state is needed
    exposes VehicleOps { model, year }
    exposes CarOps { model, year, car_doors, fuel_type }
    exposes SportsCarOps { model, year, car_doors, fuel_type, sports_horsepower, sports_turbo }
}

functional class VehicleOps {
    fn start(vehicle: &VehicleData) {
        println("Vehicle starting");
    }
    
    fn stop(vehicle: &VehicleData) {
        println("Vehicle stopping");
    }
    
    // Abstract method - no implementation at this level
    fn get_info(vehicle: &VehicleData) -> String;  // Must be implemented by concrete types
}

functional class CarOps {
    fn start(vehicle: &VehicleData) {
        println("Car engine starting");
        VehicleOps::start(vehicle);  // Explicit delegation
    }
    
    fn stop(vehicle: &VehicleData) {
        VehicleOps::stop(vehicle);   // Explicit delegation
    }
    
    fn get_info(vehicle: &VehicleData) -> String {
        format!("Car: {} ({})", vehicle.model, vehicle.year)
    }
    
    fn honk(vehicle: &VehicleData) {
        println("Beep beep!");
    }
}

functional class SportsCarOps {
    fn start(vehicle: &VehicleData) {
        println("Sports car roaring to life!");
        CarOps::start(vehicle);      // Explicit delegation chain
    }
    
    fn stop(vehicle: &VehicleData) {
        CarOps::stop(vehicle);       // Delegates through car to vehicle
    }
    
    fn get_info(vehicle: &VehicleData) -> String {
        let base_info = CarOps::get_info(vehicle);
        let hp = vehicle.sports_horsepower.unwrap_or(0);
        format!("{} - {}HP", base_info, hp)
    }
    
    fn honk(vehicle: &VehicleData) {
        CarOps::honk(vehicle);       // Explicit delegation
    }
    
    fn enable_turbo(vehicle: &mut VehicleData) {
        if let Some(ref mut turbo) = vehicle.sports_turbo {
            *turbo = true;
        }
    }
}

// Factory functions ensure proper state initialization
functional class VehicleFactory {
    fn create_sports_car(model: String, year: i32, doors: i32, hp: i32) -> VehicleData {
        VehicleData {
            model,
            year,
            car_doors: Some(doors),
            fuel_type: Some("Gasoline".to_string()),
            sports_horsepower: Some(hp),
            sports_turbo: Some(false),
        }
    }
}

// Usage demonstrates polymorphism
let sports_car = VehicleFactory::create_sports_car("Ferrari".to_string(), 2023, 2, 650);

// Different capability levels
VehicleOps::start(&sports_car);     // Basic vehicle capability
CarOps::honk(&sports_car);          // Car-specific capability  
SportsCarOps::enable_turbo(&mut sports_car);  // Sports car capability

// Polymorphic usage
let info = SportsCarOps::get_info(&sports_car);  // Uses full delegation chain
```

**Benefits over C++ version:**
- **Explicit delegation chains** - clear what each override calls
- **Unified state** - no hidden state in base classes
- **Compile-time method resolution** - no vtable lookup overhead
- **Optional components** - memory only used for active features

## Pattern Category 2: Multiple Inheritance

### C++ Multiple Inheritance

```cpp
class Drawable {
public:
    virtual void draw() = 0;
    virtual void setColor(const std::string& color) = 0;
    virtual ~Drawable() = default;
};

class Movable {
protected:
    int x, y;
public:
    Movable(int x = 0, int y = 0) : x(x), y(y) {}
    virtual void move(int dx, int dy) { x += dx; y += dy; }
    virtual std::pair<int, int> getPosition() const { return {x, y}; }
    virtual ~Movable() = default;
};

class Serializable {
public:
    virtual std::string serialize() const = 0;
    virtual void deserialize(const std::string& data) = 0;
    virtual ~Serializable() = default;
};

class Shape : public Drawable, public Movable, public Serializable {
protected:
    std::string color;
    std::string shape_type;
public:
    Shape(const std::string& type, int x = 0, int y = 0) 
        : Movable(x, y), color("black"), shape_type(type) {}
    
    void setColor(const std::string& new_color) override { 
        color = new_color; 
    }
    
    std::string serialize() const override {
        return shape_type + ":" + std::to_string(x) + "," + std::to_string(y) + ":" + color;
    }
    
    void deserialize(const std::string& data) override {
        // Parsing implementation
    }
};

class Circle : public Shape {
private:
    int radius;
public:
    Circle(int r, int x = 0, int y = 0) : Shape("Circle", x, y), radius(r) {}
    
    void draw() override {
        std::cout << "Drawing " << color << " circle at (" << x << "," << y 
                  << ") with radius " << radius << std::endl;
    }
};
```

### CPrime Translation

```cpp
class ShapeData {
    // Unified state from all inheritance branches
    position_x: i32,
    position_y: i32,
    color: String,
    shape_type: String,
    
    // Shape-specific data (optional based on actual shape)
    circle_radius: Option<i32>,
    rectangle_width: Option<i32>,
    rectangle_height: Option<i32>,
    
    // Multiple capability exposure - equivalent to multiple inheritance
    exposes DrawableOps { color, shape_type, circle_radius, rectangle_width, rectangle_height }
    exposes MovableOps { position_x, position_y }
    exposes SerializableOps { position_x, position_y, color, shape_type, circle_radius, rectangle_width, rectangle_height }
    exposes CircleOps { position_x, position_y, color, circle_radius }
    exposes RectangleOps { position_x, position_y, color, rectangle_width, rectangle_height }
}

// Separate functional classes for each capability
functional class DrawableOps {
    fn set_color(shape: &mut ShapeData, new_color: String) {
        shape.color = new_color;
    }
    
    fn draw(shape: &ShapeData);  // Abstract - implemented by concrete shapes
}

functional class MovableOps {
    fn move_by(shape: &mut ShapeData, dx: i32, dy: i32) {
        shape.position_x += dx;
        shape.position_y += dy;
    }
    
    fn get_position(shape: &ShapeData) -> (i32, i32) {
        (shape.position_x, shape.position_y)
    }
    
    fn set_position(shape: &mut ShapeData, x: i32, y: i32) {
        shape.position_x = x;
        shape.position_y = y;
    }
}

functional class SerializableOps {
    fn serialize(shape: &ShapeData) -> String {
        format!("{}:{}:{}:{}", 
            shape.shape_type,
            shape.position_x, 
            shape.position_y, 
            shape.color
        )
    }
    
    fn deserialize(shape: &mut ShapeData, data: &str) -> Result<()> {
        let parts: Vec<&str> = data.split(':').collect();
        if parts.len() >= 4 {
            shape.shape_type = parts[0].to_string();
            shape.position_x = parts[1].parse()?;
            shape.position_y = parts[2].parse()?;
            shape.color = parts[3].to_string();
            Ok(())
        } else {
            Err("Invalid serialization format")
        }
    }
}

// Concrete implementations combining all capabilities
functional class CircleOps {
    // Implements all inherited capabilities
    fn draw(shape: &ShapeData) {
        let radius = shape.circle_radius.unwrap_or(0);
        println!("Drawing {} circle at ({},{}) with radius {}", 
            shape.color, shape.position_x, shape.position_y, radius);
    }
    
    fn set_color(shape: &mut ShapeData, color: String) {
        DrawableOps::set_color(shape, color);  // Delegate to capability
    }
    
    fn move_by(shape: &mut ShapeData, dx: i32, dy: i32) {
        MovableOps::move_by(shape, dx, dy);    // Delegate to capability
    }
    
    fn get_position(shape: &ShapeData) -> (i32, i32) {
        MovableOps::get_position(shape)        // Delegate to capability
    }
    
    fn serialize(shape: &ShapeData) -> String {
        let base = SerializableOps::serialize(shape);
        let radius = shape.circle_radius.unwrap_or(0);
        format!("{}:radius:{}", base, radius)
    }
    
    fn deserialize(shape: &mut ShapeData, data: &str) -> Result<()> {
        SerializableOps::deserialize(shape, data)?;
        // Parse circle-specific data
        // ... implementation
        Ok(())
    }
    
    // Circle-specific methods
    fn set_radius(shape: &mut ShapeData, radius: i32) {
        shape.circle_radius = Some(radius);
    }
    
    fn get_radius(shape: &ShapeData) -> i32 {
        shape.circle_radius.unwrap_or(0)
    }
}

// Factory ensures proper initialization
functional class ShapeFactory {
    fn create_circle(radius: i32, x: i32, y: i32) -> ShapeData {
        ShapeData {
            position_x: x,
            position_y: y,
            color: "black".to_string(),
            shape_type: "Circle".to_string(),
            circle_radius: Some(radius),
            rectangle_width: None,
            rectangle_height: None,
        }
    }
}

// Polymorphic usage with explicit capability selection
let mut circle = ShapeFactory::create_circle(10, 5, 3);

// Use different capabilities
CircleOps::draw(&circle);                           // Drawable capability
CircleOps::move_by(&mut circle, 2, 3);             // Movable capability  
let serialized = CircleOps::serialize(&circle);    // Serializable capability
CircleOps::set_radius(&mut circle, 15);            // Circle-specific capability

// Can also use capabilities directly
MovableOps::move_by(&mut circle, 1, 1);            // Direct capability usage
DrawableOps::set_color(&mut circle, "red".to_string());
```

**Advantages over C++ multiple inheritance:**
- **No ambiguity** - each capability is explicitly named
- **Clear method resolution** - always obvious which implementation is called
- **No diamond problem** - unified state eliminates conflicts
- **Explicit delegation** - can see exactly what each method does
- **Memory efficient** - optional components only allocated when needed

## Pattern Category 3: Diamond Inheritance

### C++ Diamond Problem

```cpp
class Animal {
protected:
    std::string name;
    int age;
public:
    Animal(const std::string& n, int a) : name(n), age(a) {}
    virtual void makeSound() = 0;
    virtual void breathe() { std::cout << name << " is breathing\n"; }
    virtual ~Animal() = default;
};

class Mammal : virtual public Animal {
protected:
    bool warm_blooded = true;
public:
    Mammal(const std::string& n, int a) : Animal(n, a) {}
    virtual void giveBirth() { std::cout << "Giving birth to live young\n"; }
    void breathe() override { 
        std::cout << "Mammal breathing with lungs: ";
        Animal::breathe();
    }
};

class Winged : virtual public Animal {
protected:
    int wingspan;
public:
    Winged(const std::string& n, int a, int ws) : Animal(n, a), wingspan(ws) {}
    virtual void fly() = 0;
    void breathe() override {
        std::cout << "Flying creature breathing: ";
        Animal::breathe();
    }
};

class Bat : public Mammal, public Winged {
public:
    Bat(const std::string& n, int a, int ws) 
        : Animal(n, a), Mammal(n, a), Winged(n, a, ws) {}
    
    void makeSound() override { std::cout << "Screech!\n"; }
    void fly() override { std::cout << "Flapping wings\n"; }
    
    // Ambiguity: which breathe() to call?
    void breathe() override {
        std::cout << "Bat breathing: ";
        Animal::breathe();  // Must explicitly choose base
    }
};
```

### CPrime Diamond Resolution

```cpp
class AnimalData {
    // Single source of truth - no duplication
    name: String,
    age: i32,
    
    // Optional capabilities data
    mammalian_data: Option<MammalianTraits>,
    winged_data: Option<WingedTraits>,
    bat_data: Option<BatTraits>,
    
    // Explicit capability exposure - no ambiguity
    exposes AnimalOps { name, age }
    exposes MammalOps { name, age, mammalian_data }
    exposes WingedOps { name, age, winged_data }
    exposes BatOps { name, age, mammalian_data, winged_data, bat_data }
}

struct MammalianTraits {
    warm_blooded: bool,
    gestation_period: u32,
}

struct WingedTraits {
    wingspan: i32,
    flight_speed: f64,
}

struct BatTraits {
    echolocation_range: i32,
    hang_upside_down: bool,
}

functional class AnimalOps {
    fn breathe(animal: &AnimalData) {
        println!("{} is breathing", animal.name);
    }
    
    fn make_sound(animal: &AnimalData);  // Abstract
    
    fn get_name(animal: &AnimalData) -> &str {
        &animal.name
    }
    
    fn get_age(animal: &AnimalData) -> i32 {
        animal.age
    }
}

functional class MammalOps {
    fn breathe(animal: &AnimalData) {
        print!("Mammal breathing with lungs: ");
        AnimalOps::breathe(animal);  // Explicit delegation
    }
    
    fn give_birth(animal: &AnimalData) {
        println!("Giving birth to live young");
    }
    
    fn make_sound(animal: &AnimalData) {
        AnimalOps::make_sound(animal);  // Delegate to concrete implementation
    }
    
    fn is_warm_blooded(animal: &AnimalData) -> bool {
        animal.mammalian_data
            .as_ref()
            .map(|m| m.warm_blooded)
            .unwrap_or(false)
    }
}

functional class WingedOps {
    fn breathe(animal: &AnimalData) {
        print!("Flying creature breathing: ");
        AnimalOps::breathe(animal);  // Explicit delegation
    }
    
    fn fly(animal: &AnimalData);  // Abstract
    
    fn make_sound(animal: &AnimalData) {
        AnimalOps::make_sound(animal);  // Delegate to concrete implementation
    }
    
    fn get_wingspan(animal: &AnimalData) -> i32 {
        animal.winged_data
            .as_ref()
            .map(|w| w.wingspan)
            .unwrap_or(0)
    }
}

functional class BatOps {
    // Explicit resolution of diamond ambiguity
    fn breathe(animal: &AnimalData) {
        print!("Bat breathing: ");
        AnimalOps::breathe(animal);  // Explicitly choose base implementation
    }
    
    fn make_sound(animal: &AnimalData) {
        println!("Screech!");
    }
    
    fn fly(animal: &AnimalData) {
        println!("Flapping wings");
    }
    
    fn give_birth(animal: &AnimalData) {
        MammalOps::give_birth(animal);  // Delegate to mammal capability
    }
    
    // Can access any capability explicitly
    fn mammal_breathe(animal: &AnimalData) {
        MammalOps::breathe(animal);  // Explicit mammal breathing
    }
    
    fn winged_breathe(animal: &AnimalData) {
        WingedOps::breathe(animal);  // Explicit winged breathing
    }
    
    // Bat-specific behavior combining capabilities
    fn echolocate(animal: &AnimalData) {
        if let Some(bat_data) = &animal.bat_data {
            println!("Echolocating with range: {}", bat_data.echolocation_range);
        }
    }
    
    fn combined_ability(animal: &AnimalData) {
        // Combine mammalian and winged capabilities
        MammalOps::give_birth(animal);
        WingedOps::fly(animal);
        Self::echolocate(animal);
    }
}

// Factory ensures proper initialization
functional class AnimalFactory {
    fn create_bat(name: String, age: i32, wingspan: i32) -> AnimalData {
        AnimalData {
            name,
            age,
            mammalian_data: Some(MammalianTraits {
                warm_blooded: true,
                gestation_period: 60,
            }),
            winged_data: Some(WingedTraits {
                wingspan,
                flight_speed: 25.0,
            }),
            bat_data: Some(BatTraits {
                echolocation_range: 100,
                hang_upside_down: true,
            }),
        }
    }
}

// Usage demonstrates resolved ambiguity
let bat = AnimalFactory::create_bat("Bruce".to_string(), 3, 30);

// Different breathing implementations - no ambiguity
BatOps::breathe(&bat);          // Bat's chosen implementation
BatOps::mammal_breathe(&bat);   // Explicitly mammal breathing
BatOps::winged_breathe(&bat);   // Explicitly winged breathing

// All capabilities work
BatOps::make_sound(&bat);       // Screech!
BatOps::fly(&bat);              // Flapping wings
BatOps::give_birth(&bat);       // Mammalian capability
BatOps::echolocate(&bat);       // Bat-specific capability
BatOps::combined_ability(&bat); // Combines multiple capabilities
```

**Diamond Problem Resolution:**
- **Single state source** - no duplicate Animal state
- **Explicit method resolution** - no ambiguity about which breathe() to call
- **Capability composition** - can access any combination of capabilities explicitly
- **Clear delegation chains** - always obvious what each method calls
- **No virtual inheritance complexity** - simple compositional structure

## Pattern Category 4: Abstract Base Classes and Interfaces

### C++ Abstract Interfaces

```cpp
class Component {
public:
    virtual void update() = 0;
    virtual void render() = 0;
    virtual bool isVisible() const = 0;
    virtual void setVisible(bool visible) = 0;
    virtual ~Component() = default;
};

class Clickable {
public:
    virtual void onClick() = 0;
    virtual bool isClickable() const = 0;
    virtual ~Clickable() = default;
};

class Draggable {
public:
    virtual void onDragStart() = 0;
    virtual void onDrag(int dx, int dy) = 0;
    virtual void onDragEnd() = 0;
    virtual bool isDraggable() const = 0;
    virtual ~Draggable() = default;
};

class Button : public Component, public Clickable {
private:
    std::string text;
    bool visible = true;
    bool clickable = true;
    
public:
    Button(const std::string& t) : text(t) {}
    
    void update() override {
        // Update button state
    }
    
    void render() override {
        if (visible) {
            std::cout << "Rendering button: " << text << std::endl;
        }
    }
    
    bool isVisible() const override { return visible; }
    void setVisible(bool v) override { visible = v; }
    
    void onClick() override {
        std::cout << "Button '" << text << "' clicked!" << std::endl;
    }
    
    bool isClickable() const override { return clickable && visible; }
};

class DraggableButton : public Button, public Draggable {
private:
    bool draggable = true;
    int x = 0, y = 0;
    
public:
    DraggableButton(const std::string& t) : Button(t) {}
    
    void onDragStart() override {
        std::cout << "Started dragging button" << std::endl;
    }
    
    void onDrag(int dx, int dy) override {
        x += dx; y += dy;
        std::cout << "Dragging to (" << x << ", " << y << ")" << std::endl;
    }
    
    void onDragEnd() override {
        std::cout << "Finished dragging button" << std::endl;
    }
    
    bool isDraggable() const override { return draggable && isVisible(); }
};
```

### CPrime Interface Translation

```cpp
// Define interface contracts as traits
trait ComponentBehavior {
    fn update(&mut self);
    fn render(&self);
    fn is_visible(&self) -> bool;
    fn set_visible(&mut self, visible: bool);
}

trait ClickableBehavior {
    fn on_click(&mut self);
    fn is_clickable(&self) -> bool;
}

trait DraggableBehavior {
    fn on_drag_start(&mut self);
    fn on_drag(&mut self, dx: i32, dy: i32);
    fn on_drag_end(&mut self);
    fn is_draggable(&self) -> bool;
}

class UIComponentData {
    // Core component state
    text: String,
    visible: bool,
    position_x: i32,
    position_y: i32,
    
    // Capability-specific state
    clickable_state: Option<ClickableState>,
    draggable_state: Option<DraggableState>,
    
    // Interface exposure
    exposes ComponentOps { text, visible }
    exposes ClickableOps { text, visible, clickable_state }
    exposes DraggableOps { position_x, position_y, visible, draggable_state }
    exposes ButtonOps { text, visible, clickable_state }
    exposes DraggableButtonOps { text, visible, position_x, position_y, clickable_state, draggable_state }
}

struct ClickableState {
    enabled: bool,
    last_click_time: Option<Timestamp>,
}

struct DraggableState {
    enabled: bool,
    drag_threshold: i32,
    is_dragging: bool,
}

// Implement interfaces for specific capability combinations
impl ComponentBehavior for (UIComponentData + ComponentOps) {
    fn update(&mut self) {
        // Update component state
    }
    
    fn render(&self) {
        if self.visible {
            println!("Rendering component: {}", self.text);
        }
    }
    
    fn is_visible(&self) -> bool {
        self.visible
    }
    
    fn set_visible(&mut self, visible: bool) {
        self.visible = visible;
    }
}

impl ClickableBehavior for (UIComponentData + ClickableOps) {
    fn on_click(&mut self) {
        if let Some(ref mut clickable) = self.clickable_state {
            clickable.last_click_time = Some(current_time());
            println!("Component '{}' clicked!", self.text);
        }
    }
    
    fn is_clickable(&self) -> bool {
        self.visible && 
        self.clickable_state
            .as_ref()
            .map(|c| c.enabled)
            .unwrap_or(false)
    }
}

impl DraggableBehavior for (UIComponentData + DraggableOps) {
    fn on_drag_start(&mut self) {
        if let Some(ref mut draggable) = self.draggable_state {
            draggable.is_dragging = true;
            println!("Started dragging component");
        }
    }
    
    fn on_drag(&mut self, dx: i32, dy: i32) {
        if let Some(draggable) = &self.draggable_state {
            if draggable.is_dragging {
                self.position_x += dx;
                self.position_y += dy;
                println!("Dragging to ({}, {})", self.position_x, self.position_y);
            }
        }
    }
    
    fn on_drag_end(&mut self) {
        if let Some(ref mut draggable) = self.draggable_state {
            draggable.is_dragging = false;
            println!("Finished dragging component");
        }
    }
    
    fn is_draggable(&self) -> bool {
        self.visible &&
        self.draggable_state
            .as_ref()
            .map(|d| d.enabled)
            .unwrap_or(false)
    }
}

// Concrete implementations
functional class ButtonOps {
    fn update(button: &mut UIComponentData) {
        // Button-specific update logic
        ComponentOps::update(button);
    }
    
    fn render(button: &UIComponentData) {
        if button.visible {
            println!("Rendering button: {}", button.text);
        }
    }
    
    fn click(button: &mut UIComponentData) {
        if ClickableOps::is_clickable(button) {
            ClickableOps::on_click(button);
        }
    }
}

functional class DraggableButtonOps {
    // Implements all interfaces by delegation
    fn update(button: &mut UIComponentData) {
        ButtonOps::update(button);
    }
    
    fn render(button: &UIComponentData) {
        ButtonOps::render(button);
    }
    
    fn click(button: &mut UIComponentData) {
        ButtonOps::click(button);
    }
    
    fn start_drag(button: &mut UIComponentData) {
        if DraggableOps::is_draggable(button) {
            DraggableOps::on_drag_start(button);
        }
    }
    
    fn drag(button: &mut UIComponentData, dx: i32, dy: i32) {
        DraggableOps::on_drag(button, dx, dy);
    }
    
    fn end_drag(button: &mut UIComponentData) {
        DraggableOps::on_drag_end(button);
    }
}

// Factory for proper initialization
functional class UIFactory {
    fn create_button(text: String) -> UIComponentData {
        UIComponentData {
            text,
            visible: true,
            position_x: 0,
            position_y: 0,
            clickable_state: Some(ClickableState {
                enabled: true,
                last_click_time: None,
            }),
            draggable_state: None,
        }
    }
    
    fn create_draggable_button(text: String) -> UIComponentData {
        UIComponentData {
            text,
            visible: true,
            position_x: 0,
            position_y: 0,
            clickable_state: Some(ClickableState {
                enabled: true,
                last_click_time: None,
            }),
            draggable_state: Some(DraggableState {
                enabled: true,
                drag_threshold: 5,
                is_dragging: false,
            }),
        }
    }
}

// Generic functions work with traits
functional class UIManager<T>
where T: ComponentBehavior + ClickableBehavior + DraggableBehavior
{
    fn handle_interaction(component: &mut T, event: UIEvent) {
        match event {
            UIEvent::Click => component.on_click(),
            UIEvent::DragStart => component.on_drag_start(),
            UIEvent::Drag(dx, dy) => component.on_drag(dx, dy),
            UIEvent::DragEnd => component.on_drag_end(),
        }
        component.update();
    }
}

// Usage with multiple interfaces
let mut draggable_button = UIFactory::create_draggable_button("OK".to_string());

// Different interface usage
DraggableButtonOps::render(&draggable_button);           // Component interface
DraggableButtonOps::click(&mut draggable_button);        // Clickable interface
DraggableButtonOps::start_drag(&mut draggable_button);   // Draggable interface
DraggableButtonOps::drag(&mut draggable_button, 10, 5);  // Draggable interface
DraggableButtonOps::end_drag(&mut draggable_button);     // Draggable interface

// Generic interface usage
UIManager::handle_interaction(&mut draggable_button, UIEvent::Click);
```

**Interface Implementation Benefits:**
- **Clear contract definition** - traits define exactly what must be implemented
- **Compile-time verification** - all interface requirements checked at compile time
- **Multiple interface support** - can implement many interfaces on same type
- **Generic programming** - can write generic code over interface combinations
- **Explicit capability exposure** - access rights control which interfaces are available

## Pattern Category 5: Template Method Pattern

### C++ Template Method

```cpp
class DataProcessor {
public:
    // Template method defines algorithm structure
    void processData() {
        loadData();
        if (validateData()) {
            transformData();
            saveData();
            cleanup();
        } else {
            handleError();
        }
    }
    
protected:
    // Steps to be implemented by subclasses
    virtual void loadData() = 0;
    virtual bool validateData() = 0;
    virtual void transformData() = 0;
    virtual void saveData() = 0;
    virtual void cleanup() {}
    virtual void handleError() {
        std::cout << "Data processing error occurred" << std::endl;
    }
};

class CSVProcessor : public DataProcessor {
private:
    std::vector<std::string> data;
    
protected:
    void loadData() override {
        // Load CSV data
        std::cout << "Loading CSV data" << std::endl;
    }
    
    bool validateData() override {
        std::cout << "Validating CSV format" << std::endl;
        return !data.empty();
    }
    
    void transformData() override {
        std::cout << "Transforming CSV data" << std::endl;
    }
    
    void saveData() override {
        std::cout << "Saving processed CSV data" << std::endl;
    }
    
    void cleanup() override {
        data.clear();
        std::cout << "CSV cleanup completed" << std::endl;
    }
};

class JSONProcessor : public DataProcessor {
private:
    std::string json_data;
    
protected:
    void loadData() override {
        std::cout << "Loading JSON data" << std::endl;
    }
    
    bool validateData() override {
        std::cout << "Validating JSON format" << std::endl;
        return !json_data.empty();
    }
    
    void transformData() override {
        std::cout << "Transforming JSON data" << std::endl;
    }
    
    void saveData() override {
        std::cout << "Saving processed JSON data" << std::endl;
    }
    
    void handleError() override {
        std::cout << "JSON processing error: Invalid format" << std::endl;
    }
};
```

### CPrime Template Method Translation

```cpp
class ProcessorData {
    // Unified state for all processor types
    processing_state: ProcessingState,
    csv_data: Option<Vec<String>>,
    json_data: Option<String>,
    xml_data: Option<XmlDocument>,
    
    // Capability exposure for different processor types
    exposes BaseProcessorOps { processing_state }
    exposes CSVProcessorOps { processing_state, csv_data }
    exposes JSONProcessorOps { processing_state, json_data }
    exposes XMLProcessorOps { processing_state, xml_data }
}

struct ProcessingState {
    is_loaded: bool,
    is_validated: bool,
    is_transformed: bool,
    is_saved: bool,
    error_message: Option<String>,
}

// Base template method implementation
functional class BaseProcessorOps {
    // Template method - defines algorithm structure
    fn process_data(processor: &mut ProcessorData) -> Result<()> {
        Self::load_data(processor)?;
        
        if Self::validate_data(processor)? {
            Self::transform_data(processor)?;
            Self::save_data(processor)?;
            Self::cleanup(processor);
        } else {
            Self::handle_error(processor);
        }
        
        Ok(())
    }
    
    // Abstract methods - must be implemented by concrete processors
    fn load_data(processor: &mut ProcessorData) -> Result<()>;
    fn validate_data(processor: &ProcessorData) -> Result<bool>;
    fn transform_data(processor: &mut ProcessorData) -> Result<()>;
    fn save_data(processor: &ProcessorData) -> Result<()>;
    
    // Default implementations that can be overridden
    fn cleanup(processor: &mut ProcessorData) {
        processor.processing_state.is_loaded = false;
        processor.processing_state.is_validated = false;
        processor.processing_state.is_transformed = false;
        processor.processing_state.is_saved = false;
    }
    
    fn handle_error(processor: &mut ProcessorData) {
        println!("Data processing error occurred");
        processor.processing_state.error_message = Some("Processing failed".to_string());
    }
}

// Concrete CSV processor implementation
functional class CSVProcessorOps {
    // Template method delegates to base
    fn process_data(processor: &mut ProcessorData) -> Result<()> {
        BaseProcessorOps::process_data(processor)
    }
    
    // Implement abstract methods
    fn load_data(processor: &mut ProcessorData) -> Result<()> {
        println!("Loading CSV data");
        processor.csv_data = Some(vec!["row1".to_string(), "row2".to_string()]);
        processor.processing_state.is_loaded = true;
        Ok(())
    }
    
    fn validate_data(processor: &ProcessorData) -> Result<bool> {
        println!("Validating CSV format");
        let is_valid = processor.csv_data
            .as_ref()
            .map(|data| !data.is_empty())
            .unwrap_or(false);
        Ok(is_valid)
    }
    
    fn transform_data(processor: &mut ProcessorData) -> Result<()> {
        println!("Transforming CSV data");
        if let Some(ref mut data) = processor.csv_data {
            // Transform the data
            for row in data {
                *row = format!("processed_{}", row);
            }
        }
        processor.processing_state.is_transformed = true;
        Ok(())
    }
    
    fn save_data(processor: &ProcessorData) -> Result<()> {
        println!("Saving processed CSV data");
        // Save implementation
        Ok(())
    }
    
    // Override default cleanup
    fn cleanup(processor: &mut ProcessorData) {
        if let Some(ref mut data) = processor.csv_data {
            data.clear();
        }
        BaseProcessorOps::cleanup(processor);  // Call base cleanup
        println!("CSV cleanup completed");
    }
}

// Concrete JSON processor implementation
functional class JSONProcessorOps {
    fn process_data(processor: &mut ProcessorData) -> Result<()> {
        BaseProcessorOps::process_data(processor)
    }
    
    fn load_data(processor: &mut ProcessorData) -> Result<()> {
        println!("Loading JSON data");
        processor.json_data = Some("{'key': 'value'}".to_string());
        processor.processing_state.is_loaded = true;
        Ok(())
    }
    
    fn validate_data(processor: &ProcessorData) -> Result<bool> {
        println!("Validating JSON format");
        let is_valid = processor.json_data
            .as_ref()
            .map(|data| !data.is_empty() && data.starts_with("{"))
            .unwrap_or(false);
        Ok(is_valid)
    }
    
    fn transform_data(processor: &mut ProcessorData) -> Result<()> {
        println!("Transforming JSON data");
        if let Some(ref mut data) = processor.json_data {
            *data = format!("processed({})", data);
        }
        processor.processing_state.is_transformed = true;
        Ok(())
    }
    
    fn save_data(processor: &ProcessorData) -> Result<()> {
        println!("Saving processed JSON data");
        Ok(())
    }
    
    // Override error handling
    fn handle_error(processor: &mut ProcessorData) {
        println!("JSON processing error: Invalid format");
        processor.processing_state.error_message = Some("Invalid JSON format".to_string());
    }
}

// Factory for processor creation
functional class ProcessorFactory {
    fn create_csv_processor() -> ProcessorData {
        ProcessorData {
            processing_state: ProcessingState {
                is_loaded: false,
                is_validated: false,
                is_transformed: false,
                is_saved: false,
                error_message: None,
            },
            csv_data: None,
            json_data: None,
            xml_data: None,
        }
    }
    
    fn create_json_processor() -> ProcessorData {
        ProcessorData {
            processing_state: ProcessingState {
                is_loaded: false,
                is_validated: false,
                is_transformed: false,
                is_saved: false,
                error_message: None,
            },
            csv_data: None,
            json_data: None,
            xml_data: None,
        }
    }
}

// Usage demonstrates template method pattern
let mut csv_processor = ProcessorFactory::create_csv_processor();
CSVProcessorOps::process_data(&mut csv_processor)?;

let mut json_processor = ProcessorFactory::create_json_processor();
JSONProcessorOps::process_data(&mut json_processor)?;

// Generic processing with trait
trait DataProcessingCapability {
    fn process_data(&mut self) -> Result<()>;
}

impl DataProcessingCapability for (ProcessorData + CSVProcessorOps) {
    fn process_data(&mut self) -> Result<()> {
        CSVProcessorOps::process_data(self)
    }
}

impl DataProcessingCapability for (ProcessorData + JSONProcessorOps) {
    fn process_data(&mut self) -> Result<()> {
        JSONProcessorOps::process_data(self)
    }
}

// Generic function using template method
functional class ProcessorManager<T>
where T: DataProcessingCapability
{
    fn batch_process(processors: &mut [T]) -> Vec<Result<()>> {
        processors.iter_mut()
            .map(|p| p.process_data())
            .collect()
    }
}
```

**Template Method Pattern Benefits:**
- **Clear algorithm structure** - template method defines invariant steps
- **Explicit step implementation** - each step is clearly implemented in functional classes
- **Default behavior** - base implementations can be provided and overridden
- **Flexible customization** - specific steps can be customized while maintaining algorithm structure
- **Compile-time verification** - all required steps must be implemented

## Summary of Translation Patterns

### Universal Translation Rules

1. **State Unification**: All inheritance hierarchy state → single compositional data structure
2. **Optional Components**: Derived class features → `Option<T>` fields
3. **Capability Exposure**: Virtual methods → access rights to functional classes
4. **Explicit Delegation**: Method overrides → explicit calls to other functional classes
5. **Interface Contracts**: Abstract base classes → trait definitions
6. **Factory Creation**: Constructors → factory methods ensuring proper state initialization

### Architectural Improvements

**Eliminated Problems:**
- ❌ Diamond inheritance ambiguity
- ❌ Circular dependency potential
- ❌ Deep inheritance chains
- ❌ Hidden method resolution
- ❌ Virtual inheritance complexity

**Added Benefits:**
- ✅ Explicit method resolution
- ✅ Compile-time capability verification
- ✅ Clear delegation chains
- ✅ Unified state management
- ✅ Zero ambiguity polymorphism

### Performance Characteristics

- **Compile-time method resolution** vs runtime vtable lookup
- **Optional state allocation** vs always-allocated base class state
- **Direct function calls** vs virtual dispatch overhead
- **Cache-friendly data layout** vs pointer chasing in inheritance hierarchies

CPrime's compositional polymorphism provides equivalent functionality to all C++ inheritance patterns while eliminating architectural problems and improving performance through compile-time resolution.