"""Pure-Python reference models for the CPI-227 source/static contract.

These helpers are deliberately independent of CPython embedding.  Tests bind
their behavior to named C++ source constructs, so they document the intended
semantics without crossing the checkpoint's no-compile/no-embed boundary.
"""

from __future__ import annotations

import os
from collections.abc import Callable, Mapping
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any


class ScriptRootError(ValueError):
    """The configured script root or one of its entries is unsafe."""


UNSAFE_ACKNOWLEDGEMENT = "CPI-223-UNSAFE-DEVELOPMENT-ONLY"


@dataclass(frozen=True)
class CapabilitySurface:
    profile: str
    hook_registration: bool
    native_wrappers: bool
    mutable_bindings: bool
    exec_command: bool


def select_capability_surface(
    profile: str = "safe-static-v1",
    *,
    allow_unsafe: bool = False,
    acknowledgement: str = "",
    allow_exec: bool = False,
) -> CapabilitySurface:
    """Reference model for the fail-closed C++ capability selection gates."""

    if profile == "safe-static-v1":
        return CapabilitySurface(profile, False, False, False, False)
    if profile != "unsafe-development-v1":
        raise ValueError("unknown capability profile")
    if not allow_unsafe or acknowledgement != UNSAFE_ACKNOWLEDGEMENT:
        raise ValueError("unsafe profile acknowledgement gates are incomplete")
    return CapabilitySurface(profile, True, True, True, allow_exec)


def _reject_symlink_components(path: Path) -> None:
    current = Path(path.anchor)
    for component in path.parts[1:]:
        current /= component
        if current.is_symlink():
            raise ScriptRootError(f"symlink component is forbidden: {current}")


def resolve_script_files(configured_root: str) -> tuple[Path, ...]:
    """Reference implementation of the fail-closed C++ script-root policy."""

    if not configured_root:
        raise ScriptRootError("script root is empty")

    raw_root = Path(configured_root)
    if not raw_root.is_absolute():
        raise ScriptRootError("script root must be absolute")
    if ".." in raw_root.parts:
        raise ScriptRootError("parent traversal is forbidden")

    _reject_symlink_components(raw_root)
    try:
        canonical_root = raw_root.resolve(strict=True)
    except OSError as exc:
        raise ScriptRootError(f"script root cannot be resolved: {exc}") from exc
    if not canonical_root.is_dir():
        raise ScriptRootError("script root is not a directory")

    scripts: list[Path] = []
    for directory, directory_names, file_names in os.walk(
        canonical_root, followlinks=False
    ):
        directory_path = Path(directory)
        for name in directory_names:
            candidate = directory_path / name
            if candidate.is_symlink():
                raise ScriptRootError(f"symlink entry is forbidden: {candidate}")
        for name in file_names:
            candidate = directory_path / name
            if candidate.is_symlink():
                raise ScriptRootError(f"symlink entry is forbidden: {candidate}")
            if candidate.suffix != ".py":
                continue
            if not candidate.is_file():
                raise ScriptRootError(f"Python entry is not a regular file: {candidate}")
            resolved = candidate.resolve(strict=True)
            try:
                resolved.relative_to(canonical_root)
            except ValueError as exc:
                raise ScriptRootError(f"script escaped the configured root: {candidate}") from exc
            scripts.append(resolved)

    return tuple(sorted(scripts, key=lambda path: path.relative_to(canonical_root).as_posix()))


@dataclass
class StagedReloadModel:
    """Small model of publish-after-complete registry/namespace reload."""

    registry: dict[str, list[str]] = field(default_factory=dict)
    namespace: dict[str, Any] = field(default_factory=dict)

    def reload(
        self,
        candidates: Mapping[str, Callable[[dict[str, list[str]], dict[str, Any]], None]],
    ) -> bool:
        staged_registry: dict[str, list[str]] = {}
        staged_namespace: dict[str, Any] = {}
        try:
            for name in sorted(candidates):
                candidates[name](staged_registry, staged_namespace)
        except Exception:
            return False

        self.registry = staged_registry
        self.namespace = staged_namespace
        return True
