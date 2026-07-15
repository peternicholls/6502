import BeebCore

public enum BeebVersion: Sendable {
    public static let current = String(cString: beeb_version_string())
}
