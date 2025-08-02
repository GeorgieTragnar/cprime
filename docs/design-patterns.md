# CPrime Design Pattern Adaptations

## Overview

CPrime transforms traditional OOP design patterns through its three-class system and access rights model. Instead of inheritance-based polymorphism, CPrime achieves similar goals through composition, access rights, and module boundaries.

## Pattern Transformation Table

| Traditional Pattern | CPrime Mechanism | Key Benefits |
|-------------------|------------------|--------------|
| Factory | Module with construction control | Compile-time access verification |
| Builder | Stateless functional class | No state mutation issues |
| Adapter | Functional class translation | Zero runtime overhead |
| Observer | Channel subscriptions | Type-safe event handling |
| Strategy | Module selection | No virtual dispatch overhead |
| Decorator | Access right composition | Compile-time capability layering |
| Command | Functional class + data | Explicit state separation |
| State | Access rights + data variants | Compile-time state verification |
| Visitor | Functional class dispatch | Direct function calls |
| Template Method | Functional class composition | Clear dependency structure |

## Creational Patterns

### Factory Pattern

Traditional factories become modules with controlled construction:

```cpp
// Traditional Factory
class DatabaseConnection {
public:
    static std::unique_ptr<DatabaseConnection> create(const Config& config);
};

// CPrime Factory - Module with Access Control
mod database_factory {
    class ConnectionData {
        handle: DbHandle,
        pool_id: PoolId,
        permissions: PermissionSet,
        constructed_by: ConnectionFactory,
    }
    
    functional class ConnectionFactory {
        fn create_user_connection(config: &UserConfig) -> Result<ConnectionData> {
            let handle = establish_connection(config)?;
            Ok(ConnectionData {
                handle,
                pool_id: PoolId::user(),
                permissions: PermissionSet::USER,
            })
        }
        
        fn create_admin_connection(admin_key: &AdminKey) -> Result<ConnectionData> {
            verify_admin_credentials(admin_key)?;
            let handle = establish_privileged_connection()?;
            Ok(ConnectionData {
                handle,
                pool_id: PoolId::admin(),
                permissions: PermissionSet::ADMIN,
            })
        }
        
        fn create_readonly_connection() -> Result<ConnectionData> {
            let handle = establish_readonly_connection()?;
            Ok(ConnectionData {
                handle,
                pool_id: PoolId::readonly(),
                permissions: PermissionSet::READONLY,
            })
        }
    }
    
    // Public interface
    pub fn user_connection(config: &UserConfig) -> Result<ConnectionData> {
        ConnectionFactory::create_user_connection(config)
    }
    
    pub fn admin_connection(key: &AdminKey) -> Result<ConnectionData> {
        ConnectionFactory::create_admin_connection(key)
    }
    
    pub fn readonly_connection() -> Result<ConnectionData> {
        ConnectionFactory::create_readonly_connection()
    }
}

// Usage with compile-time access verification
let user_conn = database_factory::user_connection(&config)?;
UserOps::query(&user_conn, "SELECT * FROM public_data");
// AdminOps::delete(&user_conn, "users");  // ❌ Compile error
```

### Builder Pattern

Builders become stateless functional classes that incrementally construct data:

```cpp
// Traditional Builder
class HttpRequestBuilder {
    HttpRequest request_;
public:
    HttpRequestBuilder& setUrl(const std::string& url);
    HttpRequestBuilder& addHeader(const std::string& key, const std::string& value);
    HttpRequest build();
};

// CPrime Builder - Stateless Functional Class
class HttpRequestData {
    url: String,
    method: HttpMethod,
    headers: HashMap<String, String>,
    body: Option<Vec<u8>>,
    timeout: Duration,
    constructed_by: HttpRequestBuilder,
}

functional class HttpRequestBuilder {
    fn new() -> HttpRequestData {
        HttpRequestData {
            url: String::new(),
            method: HttpMethod::GET,
            headers: HashMap::new(),
            body: None,
            timeout: Duration::from_secs(30),
        }
    }
    
    fn with_url(mut request: HttpRequestData, url: &str) -> HttpRequestData {
        request.url = url.to_string();
        request
    }
    
    fn with_method(mut request: HttpRequestData, method: HttpMethod) -> HttpRequestData {
        request.method = method;
        request
    }
    
    fn add_header(mut request: HttpRequestData, key: &str, value: &str) -> HttpRequestData {
        request.headers.insert(key.to_string(), value.to_string());
        request
    }
    
    fn with_body(mut request: HttpRequestData, body: Vec<u8>) -> HttpRequestData {
        request.body = Some(body);
        request
    }
    
    fn with_timeout(mut request: HttpRequestData, timeout: Duration) -> HttpRequestData {
        request.timeout = timeout;
        request
    }
    
    fn build(request: HttpRequestData) -> Result<HttpRequestData> {
        if request.url.is_empty() {
            return Err("URL is required");
        }
        Ok(request)
    }
}

// Fluent interface usage
let request = HttpRequestBuilder::new()
    .with_url("https://api.example.com/users")
    .with_method(HttpMethod::POST)
    .add_header("Content-Type", "application/json")
    .add_header("Authorization", "Bearer token123")
    .with_body(json_data.into_bytes())
    .with_timeout(Duration::from_secs(10))
    .build()?;
```

### Abstract Factory

Module hierarchies replace abstract factories:

```cpp
// UI Component Factory Hierarchy
mod ui_factory {
    // Common data structures
    class ButtonData {
        text: String,
        style: ButtonStyle,
        constructed_by: ButtonOps,
    }
    
    class WindowData {
        title: String,
        size: (u32, u32),
        buttons: Vec<ButtonData>,
        constructed_by: WindowOps,
    }
    
    // Platform-specific implementations
    #[cfg(target_os = "windows")]
    mod platform {
        functional class ButtonOps {
            fn construct(text: &str, style: ButtonStyle) -> ButtonData {
                ButtonData {
                    text: text.to_string(),
                    style: ButtonStyle::Windows(style),
                }
            }
            
            fn render(button: &ButtonData) -> RenderResult {
                windows_api::create_button(&button.text, button.style)
            }
        }
        
        functional class WindowOps {
            fn construct(title: &str, size: (u32, u32)) -> WindowData {
                WindowData {
                    title: title.to_string(),
                    size,
                    buttons: Vec::new(),
                }
            }
            
            fn show(window: &WindowData) -> Result<()> {
                windows_api::show_window(window)
            }
        }
    }
    
    #[cfg(target_os = "linux")]
    mod platform {
        functional class ButtonOps {
            fn construct(text: &str, style: ButtonStyle) -> ButtonData {
                ButtonData {
                    text: text.to_string(),
                    style: ButtonStyle::Gtk(style),
                }
            }
            
            fn render(button: &ButtonData) -> RenderResult {
                gtk_api::create_button(&button.text, button.style)
            }
        }
        
        functional class WindowOps {
            fn construct(title: &str, size: (u32, u32)) -> WindowData {
                WindowData {
                    title: title.to_string(),
                    size,
                    buttons: Vec::new(),
                }
            }
            
            fn show(window: &WindowData) -> Result<()> {
                gtk_api::show_window(window)
            }
        }
    }
    
    // Public interface (platform-independent)
    pub use platform::ButtonOps;
    pub use platform::WindowOps;
}
```

## Structural Patterns

### Adapter Pattern

Functional classes translate between incompatible interfaces:

```cpp
// Legacy API
class LegacyPrinterData {
    device_id: String,
    config: LegacyConfig,
    constructed_by: LegacyPrinterOps,
}

functional class LegacyPrinterOps {
    fn print_document(printer: &LegacyPrinterData, doc: &LegacyDocument) -> Result<()> {
        // Legacy printing implementation
    }
}

// Modern API
class ModernPrinterData {
    id: PrinterId,
    capabilities: PrinterCapabilities,
    constructed_by: ModernPrinterOps,
}

functional class ModernPrinterOps {
    fn print(printer: &ModernPrinterData, job: &PrintJob) -> Result<()> {
        // Modern printing implementation
    }
}

// Adapter - Functional Class Translation
functional class PrinterAdapter {
    fn legacy_to_modern(legacy: &LegacyPrinterData) -> ModernPrinterData {
        ModernPrinterData {
            id: PrinterId::from_string(&legacy.device_id),
            capabilities: convert_legacy_config(&legacy.config),
        }
    }
    
    fn modern_to_legacy(modern: &ModernPrinterData) -> LegacyPrinterData {
        LegacyPrinterData {
            device_id: modern.id.to_string(),
            config: convert_modern_capabilities(&modern.capabilities),
        }
    }
    
    fn print_on_legacy(legacy: &LegacyPrinterData, job: &PrintJob) -> Result<()> {
        let legacy_doc = convert_print_job_to_legacy_document(job)?;
        LegacyPrinterOps::print_document(legacy, &legacy_doc)
    }
    
    fn print_on_modern_via_legacy(legacy: &LegacyPrinterData, job: &PrintJob) -> Result<()> {
        let modern = Self::legacy_to_modern(legacy);
        ModernPrinterOps::print(&modern, job)
    }
}

// Usage
let legacy_printer = get_legacy_printer();
let print_job = create_print_job();

// Adapt legacy to modern interface
PrinterAdapter::print_on_modern_via_legacy(&legacy_printer, &print_job)?;
```

### Decorator Pattern

Access rights composition replaces traditional decorators:

```cpp
// Traditional Decorator
class FileStream {
    virtual void write(const std::string& data) = 0;
};

class EncryptedFileStream : public FileStream {
    std::unique_ptr<FileStream> base_;
    void write(const std::string& data) override;
};

// CPrime Decorator - Access Rights Composition
class FileData {
    handle: FileHandle,
    path: String,
    
    // Multiple access interfaces
    exposes BasicFileOps { handle }
    exposes EncryptedFileOps { handle }
    exposes CompressedFileOps { handle }
    exposes BufferedFileOps { handle }
}

functional class BasicFileOps {
    fn write(file: &FileData, data: &[u8]) -> Result<()> {
        raw_write(file.handle, data)
    }
    
    fn read(file: &FileData, buffer: &mut [u8]) -> Result<usize> {
        raw_read(file.handle, buffer)
    }
}

functional class EncryptedFileOps {
    fn write(file: &FileData, data: &[u8]) -> Result<()> {
        let encrypted = encrypt(data, get_encryption_key())?;
        BasicFileOps::write(file, &encrypted)
    }
    
    fn read(file: &FileData, buffer: &mut [u8]) -> Result<usize> {
        let mut encrypted_buffer = vec![0u8; buffer.len() * 2]; // Encryption overhead
        let bytes_read = BasicFileOps::read(file, &mut encrypted_buffer)?;
        let decrypted = decrypt(&encrypted_buffer[..bytes_read], get_encryption_key())?;
        
        let copy_len = std::cmp::min(buffer.len(), decrypted.len());
        buffer[..copy_len].copy_from_slice(&decrypted[..copy_len]);
        Ok(copy_len)
    }
}

functional class CompressedFileOps {
    fn write(file: &FileData, data: &[u8]) -> Result<()> {
        let compressed = compress(data)?;
        BasicFileOps::write(file, &compressed)
    }
    
    fn read(file: &FileData, buffer: &mut [u8]) -> Result<usize> {
        let mut compressed_buffer = vec![0u8; buffer.len() * 2];
        let bytes_read = BasicFileOps::read(file, &mut compressed_buffer)?;
        let decompressed = decompress(&compressed_buffer[..bytes_read])?;
        
        let copy_len = std::cmp::min(buffer.len(), decompressed.len());
        buffer[..copy_len].copy_from_slice(&decompressed[..copy_len]);
        Ok(copy_len)
    }
}

// Composition of decorations
functional class EncryptedCompressedFileOps {
    fn write(file: &FileData, data: &[u8]) -> Result<()> {
        let compressed = compress(data)?;
        EncryptedFileOps::write(file, &compressed)
    }
    
    fn read(file: &FileData, buffer: &mut [u8]) -> Result<usize> {
        let mut temp_buffer = vec![0u8; buffer.len() * 4]; // Extra space for encryption + compression
        let bytes_read = EncryptedFileOps::read(file, &mut temp_buffer)?;
        let decompressed = decompress(&temp_buffer[..bytes_read])?;
        
        let copy_len = std::cmp::min(buffer.len(), decompressed.len());
        buffer[..copy_len].copy_from_slice(&decompressed[..copy_len]);
        Ok(copy_len)
    }
}

// Usage with different decoration levels
let file = open_file("data.txt")?;
defer close_file(&mut file);

// Basic operations
BasicFileOps::write(&file, b"Hello, World!")?;

// Encrypted operations
EncryptedFileOps::write(&file, b"Secret data")?;

// Compressed operations  
CompressedFileOps::write(&file, b"Large data to compress")?;

// Combined operations
EncryptedCompressedFileOps::write(&file, b"Secret compressed data")?;
```

### Facade Pattern

Modules naturally provide facade interfaces:

```cpp
// Complex subsystem
mod graphics_subsystem {
    class RendererData { ... }
    class ShaderData { ... }
    class TextureData { ... }
    class BufferData { ... }
    
    functional class RendererOps { ... }
    functional class ShaderOps { ... }
    functional class TextureOps { ... }
    functional class BufferOps { ... }
}

// Facade module - simplified interface
mod graphics {
    use graphics_subsystem::*;
    
    class SceneData {
        renderer: RendererData,
        shaders: HashMap<String, ShaderData>,
        textures: HashMap<String, TextureData>,
        constructed_by: SceneOps,
    }
    
    pub functional class SceneOps {
        pub fn construct() -> Result<SceneData> {
            let renderer = RendererOps::construct()?;
            Ok(SceneData {
                renderer,
                shaders: HashMap::new(),
                textures: HashMap::new(),
            })
        }
        
        pub fn load_model(scene: &mut SceneData, path: &str) -> Result<ModelId> {
            // Coordinates multiple subsystems
            let texture = TextureOps::load_from_file(&format!("{}.texture", path))?;
            let shader = ShaderOps::load_from_file(&format!("{}.shader", path))?;
            let model = ModelOps::load_from_file(path)?;
            
            let texture_id = TextureOps::register(&mut scene.renderer, texture)?;
            let shader_id = ShaderOps::register(&mut scene.renderer, shader)?;
            let model_id = ModelOps::register(&mut scene.renderer, model, texture_id, shader_id)?;
            
            Ok(model_id)
        }
        
        pub fn render_frame(scene: &SceneData) -> Result<()> {
            RendererOps::begin_frame(&scene.renderer)?;
            RendererOps::render_all(&scene.renderer)?;
            RendererOps::end_frame(&scene.renderer)?;
            Ok(())
        }
    }
}

// Simple public interface
let mut scene = graphics::SceneOps::construct()?;
let model = graphics::SceneOps::load_model(&mut scene, "assets/character.obj")?;
graphics::SceneOps::render_frame(&scene)?;
```

## Behavioral Patterns

### Observer Pattern

Channels replace traditional observer notifications:

```cpp
// Traditional Observer
class Observable {
    std::vector<Observer*> observers_;
public:
    void addObserver(Observer* obs);
    void notifyObservers(const Event& event);
};

// CPrime Observer - Channel-Based
class EventSourceData {
    channels: Vec<Sender<Event>>,
    constructed_by: EventSourceOps,
}

functional class EventSourceOps {
    fn construct() -> EventSourceData {
        EventSourceData {
            channels: Vec::new(),
        }
    }
    
    fn subscribe(source: &mut EventSourceData) -> Receiver<Event> {
        let (tx, rx) = channel::<Event>();
        source.channels.push(tx);
        rx
    }
    
    fn notify_all(source: &EventSourceData, event: Event) {
        for channel in &source.channels {
            // Non-blocking send - observers that can't keep up are dropped
            let _ = channel.try_send(event.clone());
        }
    }
    
    fn remove_closed_channels(source: &mut EventSourceData) {
        source.channels.retain(|ch| !ch.is_closed());
    }
}

// Observer implementations
fn ui_observer(events: Receiver<Event>) {
    spawn async move {
        loop {
            match events.recv().await {
                Ok(Event::UserAction(action)) => {
                    update_ui(action);
                }
                Ok(Event::DataChanged(data)) => {
                    refresh_display(data);
                }
                Ok(_) => {}, // Ignore other events
                Err(_) => break, // Channel closed
            }
        }
    };
}

fn logging_observer(events: Receiver<Event>) {
    spawn async move {
        loop {
            match events.recv().await {
                Ok(event) => {
                    log::info("Event occurred: {:?}", event);
                }
                Err(_) => break,
            }
        }
    };
}

// Usage
let mut event_source = EventSourceOps::construct();

// Subscribe observers
let ui_events = EventSourceOps::subscribe(&mut event_source);
let log_events = EventSourceOps::subscribe(&mut event_source);

// Start observers
ui_observer(ui_events);
logging_observer(log_events);

// Emit events
EventSourceOps::notify_all(&event_source, Event::UserAction(Action::Click));
EventSourceOps::notify_all(&event_source, Event::DataChanged(new_data));
```

### Strategy Pattern

Module selection replaces strategy objects:

```cpp
// Traditional Strategy
class SortStrategy {
public:
    virtual void sort(std::vector<int>& data) = 0;
};

class QuickSort : public SortStrategy { ... };
class MergeSort : public SortStrategy { ... };

// CPrime Strategy - Module Selection
class SortableData<T> {
    items: Vec<T>,
    constructed_by: SortOps,
}

// Different strategy implementations
mod quick_sort {
    pub functional class SortOps<T: Comparable> {
        pub fn sort(data: &mut SortableData<T>) {
            quick_sort_impl(&mut data.items);
        }
    }
}

mod merge_sort {
    pub functional class SortOps<T: Comparable> {
        pub fn sort(data: &mut SortableData<T>) {
            merge_sort_impl(&mut data.items);
        }
    }
}

mod heap_sort {
    pub functional class SortOps<T: Comparable> {
        pub fn sort(data: &mut SortableData<T>) {
            heap_sort_impl(&mut data.items);
        }
    }
}

// Strategy selection based on data characteristics
functional class AdaptiveSorter<T: Comparable> {
    fn sort_optimally(data: &mut SortableData<T>) {
        match data.items.len() {
            0..=10 => {
                // Insertion sort for small arrays
                insertion_sort::SortOps::sort(data);
            }
            11..=1000 => {
                // Quick sort for medium arrays
                quick_sort::SortOps::sort(data);
            }
            _ => {
                // Merge sort for large arrays (stable, predictable performance)
                merge_sort::SortOps::sort(data);
            }
        }
    }
    
    fn sort_with_strategy(data: &mut SortableData<T>, strategy: SortStrategy) {
        match strategy {
            SortStrategy::Quick => quick_sort::SortOps::sort(data),
            SortStrategy::Merge => merge_sort::SortOps::sort(data),
            SortStrategy::Heap => heap_sort::SortOps::sort(data),
        }
    }
}

enum SortStrategy {
    Quick,
    Merge,
    Heap,
}

// Usage
let mut data = SortOps::construct(vec![5, 2, 8, 1, 9]);

// Automatic strategy selection
AdaptiveSorter::sort_optimally(&mut data);

// Manual strategy selection
AdaptiveSorter::sort_with_strategy(&mut data, SortStrategy::Merge);
```

### Command Pattern

Functional classes separate command data from execution:

```cpp
// Traditional Command
class Command {
public:
    virtual void execute() = 0;
    virtual void undo() = 0;
};

// CPrime Command - Data + Operations Separation
class FileCommandData {
    operation: FileOperation,
    path: String,
    content: Option<Vec<u8>>,
    backup: Option<Vec<u8>>,
    constructed_by: FileCommandOps,
}

enum FileOperation {
    Create,
    Delete,
    Modify,
    Move { from: String, to: String },
}

functional class FileCommandOps {
    fn create_file(path: &str, content: Vec<u8>) -> FileCommandData {
        FileCommandData {
            operation: FileOperation::Create,
            path: path.to_string(),
            content: Some(content),
            backup: None,
        }
    }
    
    fn delete_file(path: &str) -> FileCommandData {
        FileCommandData {
            operation: FileOperation::Delete,
            path: path.to_string(),
            content: None,
            backup: None,
        }
    }
    
    fn modify_file(path: &str, new_content: Vec<u8>) -> FileCommandData {
        FileCommandData {
            operation: FileOperation::Modify,
            path: path.to_string(),
            content: Some(new_content),
            backup: None,
        }
    }
    
    fn execute(command: &mut FileCommandData) -> Result<()> {
        match &command.operation {
            FileOperation::Create => {
                if let Some(content) = &command.content {
                    fs::write(&command.path, content)?;
                }
            }
            FileOperation::Delete => {
                if fs::exists(&command.path) {
                    command.backup = Some(fs::read(&command.path)?);
                    fs::remove_file(&command.path)?;
                }
            }
            FileOperation::Modify => {
                if fs::exists(&command.path) {
                    command.backup = Some(fs::read(&command.path)?);
                    if let Some(content) = &command.content {
                        fs::write(&command.path, content)?;
                    }
                }
            }
            FileOperation::Move { from, to } => {
                fs::rename(from, to)?;
            }
        }
        Ok(())
    }
    
    fn undo(command: &FileCommandData) -> Result<()> {
        match &command.operation {
            FileOperation::Create => {
                if fs::exists(&command.path) {
                    fs::remove_file(&command.path)?;
                }
            }
            FileOperation::Delete => {
                if let Some(backup) = &command.backup {
                    fs::write(&command.path, backup)?;
                }
            }
            FileOperation::Modify => {
                if let Some(backup) = &command.backup {
                    fs::write(&command.path, backup)?;
                }
            }
            FileOperation::Move { from, to } => {
                fs::rename(to, from)?;
            }
        }
        Ok(())
    }
}

// Command queue for undo/redo functionality
class CommandHistoryData {
    executed: Vec<FileCommandData>,
    undo_stack: Vec<FileCommandData>,
    constructed_by: CommandHistoryOps,
}

functional class CommandHistoryOps {
    fn construct() -> CommandHistoryData {
        CommandHistoryData {
            executed: Vec::new(),
            undo_stack: Vec::new(),
        }
    }
    
    fn execute_command(history: &mut CommandHistoryData, mut command: FileCommandData) -> Result<()> {
        FileCommandOps::execute(&mut command)?;
        history.executed.push(command);
        history.undo_stack.clear(); // Clear redo stack
        Ok(())
    }
    
    fn undo(history: &mut CommandHistoryData) -> Result<()> {
        if let Some(command) = history.executed.pop() {
            FileCommandOps::undo(&command)?;
            history.undo_stack.push(command);
        }
        Ok(())
    }
    
    fn redo(history: &mut CommandHistoryData) -> Result<()> {
        if let Some(mut command) = history.undo_stack.pop() {
            FileCommandOps::execute(&mut command)?;
            history.executed.push(command);
        }
        Ok(())
    }
}

// Usage
let mut history = CommandHistoryOps::construct();

// Execute commands
let create_cmd = FileCommandOps::create_file("test.txt", b"Hello, World!".to_vec());
CommandHistoryOps::execute_command(&mut history, create_cmd)?;

let modify_cmd = FileCommandOps::modify_file("test.txt", b"Hello, CPrime!".to_vec());
CommandHistoryOps::execute_command(&mut history, modify_cmd)?;

// Undo/redo
CommandHistoryOps::undo(&mut history)?;  // Revert modification
CommandHistoryOps::undo(&mut history)?;  // Remove file
CommandHistoryOps::redo(&mut history)?;  // Recreate file
```

## New Patterns Enabled by CPrime

### Access Escalation Pattern

Request higher privileges at runtime when needed:

```cpp
class EscalationContextData {
    current_access: AccessLevel,
    requested_access: Option<AccessLevel>,
    justification: String,
    granted_until: Option<Timestamp>,
    constructed_by: EscalationOps,
}

functional class EscalationOps {
    fn request_escalation(
        context: &mut EscalationContextData,
        target_access: AccessLevel,
        justification: &str
    ) -> Result<()> {
        if context.current_access >= target_access {
            return Ok(()); // Already have sufficient access
        }
        
        context.requested_access = Some(target_access);
        context.justification = justification.to_string();
        
        // Audit the request
        audit_log::record_escalation_request(
            context.current_access,
            target_access,
            justification
        );
        
        Ok(())
    }
    
    fn grant_escalation(
        context: &mut EscalationContextData,
        admin_key: &AdminKey,
        duration: Duration
    ) -> Result<EscalatedContextData> {
        verify_admin_key(admin_key)?;
        
        if let Some(requested) = context.requested_access {
            let expiry = current_time() + duration;
            
            audit_log::record_escalation_granted(
                context.current_access,
                requested,
                admin_key.user_id(),
                expiry
            );
            
            Ok(EscalatedContextData {
                original_access: context.current_access,
                escalated_access: requested,
                expires_at: expiry,
                justification: context.justification.clone(),
            })
        } else {
            Err("No escalation requested")
        }
    }
}

class EscalatedContextData {
    original_access: AccessLevel,
    escalated_access: AccessLevel,
    expires_at: Timestamp,
    justification: String,
    constructed_by: EscalatedOps,
}

functional class EscalatedOps {
    fn with_escalated_access<T>(
        context: &EscalatedContextData,
        operation: impl FnOnce() -> Result<T>
    ) -> Result<T> {
        if current_time() > context.expires_at {
            return Err("Escalated access has expired");
        }
        
        // Temporarily elevate access level
        let _guard = set_thread_access_level(context.escalated_access);
        
        audit_log::record_escalated_operation_start(&context.justification);
        let result = operation();
        audit_log::record_escalated_operation_end(&result);
        
        result
    }
}
```

### Capability Composition Pattern

Combine multiple access interfaces for complex operations:

```cpp
class MultiCapabilityData<DB, FS, NET> {
    database: DB,
    filesystem: FS,
    network: NET,
    constructed_by: MultiCapabilityOps,
}

functional class MultiCapabilityOps<DB, FS, NET> {
    fn construct(db: DB, fs: FS, net: NET) -> MultiCapabilityData<DB, FS, NET> {
        MultiCapabilityData {
            database: db,
            filesystem: fs,
            network: net,
        }
    }
    
    fn sync_data_to_remote(
        caps: &MultiCapabilityData<DB, FS, NET>
    ) -> Result<()> 
    where
        DB: DatabaseCapability,
        FS: FilesystemCapability,
        NET: NetworkCapability,
    {
        // Requires all three capabilities
        let data = caps.database.export_data()?;
        let temp_file = caps.filesystem.create_temp_file()?;
        caps.filesystem.write_file(&temp_file, &data)?;
        
        let compressed = caps.filesystem.compress_file(&temp_file)?;
        caps.network.upload_file(&compressed, "backup.tar.gz")?;
        caps.filesystem.delete_file(&temp_file)?;
        
        Ok(())
    }
}

// Usage requires providing all necessary capabilities
let db_conn = database::create_admin_connection(&admin_key)?;
let fs_access = filesystem::create_temp_access(&temp_dir)?;
let net_client = network::create_authenticated_client(&auth_token)?;

let multi_caps = MultiCapabilityOps::construct(db_conn, fs_access, net_client);
MultiCapabilityOps::sync_data_to_remote(&multi_caps)?;
```

### Module Boundary Pattern

Each module defines its own view of shared data:

```cpp
// Shared data visible to multiple modules
class UserData {
    id: UserId,
    name: String,
    email: String,
    password_hash: String,
    created_at: Timestamp,
    last_login: Option<Timestamp>,
    
    // Different modules see different fields
    exposes PublicUserOps { id, name, created_at }
    exposes AuthenticationOps { id, password_hash, last_login }
    exposes AdminUserOps { id, name, email, password_hash, created_at, last_login }
}

mod public_api {
    use super::UserData;
    
    pub functional class PublicUserOps {
        pub fn get_display_name(user: &UserData) -> &str {
            &user.name  // Can only access public fields
        }
        
        pub fn get_join_date(user: &UserData) -> Timestamp {
            user.created_at
        }
        
        // user.email not accessible here - compile error
    }
}

mod authentication {
    use super::UserData;
    
    pub functional class AuthenticationOps {
        pub fn verify_password(user: &UserData, password: &str) -> bool {
            verify_hash(password, &user.password_hash)  // Can access password_hash
        }
        
        pub fn update_last_login(user: &mut UserData) {
            user.last_login = Some(current_time());  // Can modify last_login
        }
        
        // user.email not accessible - different module boundary
    }
}

mod admin {
    use super::UserData;
    
    pub functional class AdminUserOps {
        pub fn get_full_info(user: &UserData) -> UserInfo {
            UserInfo {
                id: user.id,
                name: user.name.clone(),
                email: user.email.clone(),  // Admin can see email
                created_at: user.created_at,
                last_login: user.last_login,
            }
        }
        
        pub fn change_password(user: &mut UserData, new_password: &str) {
            user.password_hash = hash_password(new_password);  // Admin can change password
        }
    }
}

// Each module has its own limited view of the same data
let user = load_user(user_id)?;

// Public operations
let display_name = public_api::PublicUserOps::get_display_name(&user);

// Authentication operations  
let is_valid = authentication::AuthenticationOps::verify_password(&user, password);

// Admin operations (requires admin privileges)
let full_info = admin::AdminUserOps::get_full_info(&user);
```

CPrime's design patterns provide the same organizational benefits as traditional OOP patterns while offering better performance, clearer dependencies, and compile-time verification of access rights and capabilities.