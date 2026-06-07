from __future__ import annotations

from dataclasses import dataclass, field

from .z_serial_view_model import RenderedLine, TransportFactory, ZSerialWindowViewModel
from ..serial_config import SerialWorkspaceProfile


@dataclass
class ZSerialTab:
    tab_id: str
    title: str
    view_model: ZSerialWindowViewModel


@dataclass
class ZSerialTabManager:
    workspace: SerialWorkspaceProfile
    transport_factory: TransportFactory | None = None
    tabs: list[ZSerialTab] = field(default_factory=list)
    active_index: int = 0
    _next_id: int = 1

    def add_tab(self, title: str | None = None) -> ZSerialTab:
        tab_id = f"tab-{self._next_id}"
        self._next_id += 1
        tab = ZSerialTab(
            tab_id=tab_id,
            title=title or f"Serial {len(self.tabs) + 1}",
            view_model=ZSerialWindowViewModel(workspace=self.workspace, transport_factory=self.transport_factory),
        )
        self.tabs.append(tab)
        self.active_index = len(self.tabs) - 1
        return tab

    def close_tab(self, index: int) -> ZSerialTab:
        tab = self.tabs.pop(index)
        tab.view_model.close_port()
        if not self.tabs:
            self.active_index = 0
        else:
            self.active_index = min(index, len(self.tabs) - 1)
        return tab

    def set_active_index(self, index: int) -> None:
        if index < 0 or index >= len(self.tabs):
            raise IndexError("tab index out of range")
        self.active_index = index

    def active_tab(self) -> ZSerialTab:
        if not self.tabs:
            return self.add_tab()
        return self.tabs[self.active_index]

    def poll_all(self) -> dict[str, tuple[RenderedLine, ...]]:
        rendered: dict[str, tuple[RenderedLine, ...]] = {}
        for tab in self.tabs:
            if tab.view_model.is_open:
                lines = tab.view_model.poll_rx()
                if lines:
                    rendered[tab.tab_id] = lines
        return rendered

    def close_all(self) -> None:
        for tab in tuple(self.tabs):
            tab.view_model.close_port()
        self.tabs.clear()
        self.active_index = 0
