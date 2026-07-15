import BeebCore

/// Version information for the linked Beeb core.
public enum BeebVersion: Sendable {
    /// Semantic version reported by the C core, copied into Swift storage.
    public static let current = String(cString: beeb_version_string())
}
