#!/usr/bin/env python3
# Copyright (c) Facebook, Inc. and its affiliates.
#
# This software may be used and distributed according to the terms of the
# GNU General Public License version 2.

# pyre-strict

import abc
import getpass
import json
import logging
import platform
import random
import socket
import subprocess
import time
from pathlib import Path
from typing import Dict, List, Optional, Tuple, Union

from . import version


log: logging.Logger = logging.getLogger(__name__)

_session_id: Optional[int] = None


class TelemetrySample(abc.ABC):
    @abc.abstractmethod
    def add_int(self, name: str, value: int) -> "TelemetrySample":
        raise NotImplementedError()

    @abc.abstractmethod
    def add_string(self, name: str, value: str) -> "TelemetrySample":
        raise NotImplementedError()

    @abc.abstractmethod
    def add_double(self, name: str, value: float) -> "TelemetrySample":
        raise NotImplementedError()

    def add_bool(self, name: str, value: bool) -> "TelemetrySample":
        return self.add_int(name, int(value))

    def add_fields(self, **kwargs: Union[bool, int, str, float]) -> "TelemetrySample":
        for name, value in kwargs.items():
            if isinstance(value, bool):
                self.add_bool(name, value)
            elif isinstance(value, str):
                self.add_string(name, value)
            elif isinstance(value, int):
                self.add_int(name, value)
            elif isinstance(value, float):
                self.add_double(name, value)
            else:  # unsupported type
                log.error(
                    f"unsupported value type {type(value)} passed to add_fields()"
                )
        return self

    def log(self) -> None:
        """Log the sample to the telemetry data store."""
        self.add_int("time", int(time.time()))
        try:
            self._log_impl()
        except Exception as ex:
            log.warning(f"error logging telemetry sample: {ex}")

    @abc.abstractmethod
    def _log_impl(self) -> None:
        raise NotImplementedError()


class TelemetryLogger(abc.ABC):
    """TelemetryLogger provides an interface for logging structured log events.
    """

    session_id: int
    user: str
    hostname: str
    os: str
    os_version: str
    eden_version: str

    def __init__(self) -> None:
        self.session_id = get_session_id()
        try:
            self.user = getpass.getuser()
        except Exception as ex:
            log.warning(f"error determining username for telemetry logging: {ex}")
            self.user = ""
        try:
            self.hostname = socket.gethostname()
        except Exception as ex:
            log.warning(f"error determining hostname for telemetry logging: {ex}")
            self.hostname = ""
        try:
            self.os, self.os_version = get_os_and_ver()
        except Exception as ex:
            log.warning(f"error determining OS information for telemetry logging: {ex}")
            self.os = ""
            self.os_version = ""
        try:
            self.eden_version = version.get_current_version()
        except Exception as ex:
            log.warning(f"error determining EdenFS version for telemetry logging: {ex}")
            self.eden_version = ""

    def new_sample(
        self, event_type: str, **kwargs: Union[bool, int, str, float]
    ) -> TelemetrySample:
        sample = self._create_sample()
        sample.add_string("type", event_type)
        sample.add_int("session_id", self.session_id)
        sample.add_string("user", self.user)
        sample.add_string("host", self.hostname)
        sample.add_string("os", self.os)
        sample.add_string("osver", self.os_version)
        sample.add_string("edenver", self.eden_version)
        sample.add_fields(**kwargs)
        return sample

    def log(self, event_type: str, **kwargs: Union[bool, int, str, float]) -> None:
        self.new_sample(event_type, **kwargs).log()

    @abc.abstractmethod
    def _create_sample(self) -> TelemetrySample:
        raise NotImplementedError()


class JsonTelemetrySample(TelemetrySample):
    def __init__(self, logger: "BaseJsonTelemetryLogger") -> None:
        super().__init__()
        self.ints: Dict[str, int] = {}
        self.strings: Dict[str, str] = {}
        self.doubles: Dict[str, float] = {}
        self.logger: "BaseJsonTelemetryLogger" = logger

    def add_int(self, name: str, value: int) -> "JsonTelemetrySample":
        self.ints[name] = value
        return self

    def add_string(self, name: str, value: str) -> "JsonTelemetrySample":
        self.strings[name] = value
        return self

    def add_double(self, name: str, value: float) -> "JsonTelemetrySample":
        self.doubles[name] = value
        return self

    def get_json(self) -> str:
        data: Dict[str, Union[Dict[str, str], Dict[str, int], Dict[str, float]]] = {}
        data["int"] = self.ints
        data["normal"] = self.strings
        if self.doubles:
            data["double"] = self.doubles
        return json.dumps(data)

    def _log_impl(self) -> None:
        self.logger.log_sample(self.get_json())


class BaseJsonTelemetryLogger(TelemetryLogger):
    def _create_sample(self) -> TelemetrySample:
        return JsonTelemetrySample(self)

    @abc.abstractmethod
    def log_sample(self, sample_data: str) -> None:
        raise NotImplementedError()


class ExternalTelemetryLogger(BaseJsonTelemetryLogger):
    """A TelemetryLogger that uses an external process to log samples.
    """

    cmd: List[str]

    def __init__(self, cmd: List[str]) -> None:
        super().__init__()
        self.cmd = cmd[:]

    def log_sample(self, sample_data: str) -> None:
        cmd = self.cmd + [sample_data]
        try:
            rc = subprocess.call(cmd)
            if rc != 0:
                log.warning(f"telemetry log command returned non-zero exit code {rc}")
        except Exception as ex:
            log.warning(f"error calling telemetry log command: {ex}")


class LocalTelemetryLogger(BaseJsonTelemetryLogger):
    """A TelemetryLogger that logs samples to a local file.
    This is primarily useful just for debugging during development.
    """

    path: Path

    def __init__(self, path: Union[str, Path]) -> None:
        super().__init__()
        self.path = Path(path)

    def log_sample(self, sample_data: str) -> None:
        with self.path.open("a") as f:
            f.write(sample_data + "\n")


class NullTelemetrySample(TelemetrySample):
    def add_int(self, name: str, value: int) -> "NullTelemetrySample":
        pass

    def add_string(self, name: str, value: str) -> "NullTelemetrySample":
        pass

    def add_double(self, name: str, value: float) -> "NullTelemetrySample":
        pass

    def add_bool(self, name: str, value: bool) -> "NullTelemetrySample":
        pass

    def add_fields(
        self, **kwargs: Union[bool, int, str, float]
    ) -> "NullTelemetrySample":
        pass

    def _log_impl(self) -> None:
        pass


class NullTelemetryLogger(TelemetryLogger):
    """A TelemetryLogger that discards all samples.
    """

    def _create_sample(self) -> TelemetrySample:
        return NullTelemetrySample()


def get_session_id() -> int:
    global _session_id
    sid = _session_id
    if sid is None:
        sid = random.randrange(2 ** 32)
        _session_id = sid
    return sid


def get_os_and_ver() -> Tuple[str, str]:
    os = platform.system()
    if os == "Darwin":
        os = "macOS"
    if os == "":
        os = "unknown"

    ver = platform.release()
    if ver == "":
        ver = "unknown"

    return os, ver
