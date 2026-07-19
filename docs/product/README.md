# Product documentation strand

This strand describes the wider Beeb6502 application: the people it serves and
the Machine, Media and Editor experiences.

- [Machine application delivery plan](MACHINE_DELIVERY_PLAN.md) defines the
  sole programme direction, bounded specification order and evidence gates.
- [Vision](VISION.md) defines supporting durable intent and success qualities.
- [Product capability catalogue](ROADMAP.md) records possible user-facing scope
  without assigning priority or order.
- [Legacy decision register](LEGACY_DECISIONS.md) interprets historical design
  material without creating current commitments.

Only the delivery plan may create committed requirements for the emulator. The
supporting product documents do not define internal architecture or claim that
a capability is implemented. The emulator strand owns those concerns:

- [Core phase catalogue](../CORE_ROADMAP.md)
- [Implementation status](../STATUS.md)
- [Core architecture](../ARCHITECTURE.md)
- [Technical references](../REFERENCES.md)

When the strands interact, the delivery plan states the selected outcome. The
supporting core catalogue and architecture constrain its implementation, while
`STATUS.md` remains the only authority for what has actually been verified.
