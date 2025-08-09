class Connection {
    handle: DbHandle,
    runtime exposes UserOps { handle }
    exposes AdminOps { handle }
}

functional class FileOps {
    fn read(data: &mut FileData) -> Result<usize> {
        defer FileOps::cleanup(&data);
        // implementation
    }
}

union runtime ConnectionSpace {
    UserConn(Connection<UserOps>),
    AdminConn(Connection<AdminOps>)
}