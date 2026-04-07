#!/usr/bin/env python3
import json
import time
from pathlib import Path
from typing import Any, Dict, Iterable, List, Optional, Sequence, Union

import zmq


JsonPayload = Union[Dict[str, Any], List[Any]]


class LabJackProtocolSender:
    """Context-managed ZMQ PUSH client for LabJack protocol commands.

    Typical usage:

        with LabJackProtocolSender() as sender:
            sender.send_operations([
                sender.make_operation(300, [1, 0, 1, 0, 0, 0, 0, 0]),
                sender.make_operation(300, [0, 1, 0, 1, 0, 0, 0, 0]),
            ])
    """

    def __init__(
        self,
        host: str = "127.0.0.1",
        port: int = 33090,
        *,
        connect_wait_ms: int = 100,
        linger_ms: int = 2000,
        send_timeout_ms: int = 2000,
        endpoint: Optional[str] = None,
        context: Optional[zmq.Context] = None,
    ) -> None:
        self._endpoint = endpoint or f"tcp://{host}:{port}"
        self._connect_wait_ms = connect_wait_ms
        self._linger_ms = linger_ms
        self._send_timeout_ms = send_timeout_ms

        self._context = context or zmq.Context.instance()
        self._socket: Optional[zmq.Socket] = None

    @property
    def endpoint(self) -> str:
        return self._endpoint

    @property
    def is_connected(self) -> bool:
        return self._socket is not None

    def __enter__(self) -> "LabJackProtocolSender":
        self.connect()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb) -> None:
        self.close()

    def connect(self) -> None:
        if self._socket is not None:
            return
        socket = self._context.socket(zmq.PUSH)
        socket.setsockopt(zmq.SNDTIMEO, self._send_timeout_ms)
        socket.setsockopt(zmq.LINGER, self._linger_ms)
        socket.connect(self._endpoint)
        if self._connect_wait_ms > 0:
            time.sleep(self._connect_wait_ms / 1000.0)
        self._socket = socket

    def close(self) -> None:
        if self._socket is None:
            return
        self._socket.close()
        self._socket = None

    def send_payload(self, payload: JsonPayload) -> None:
        if self._socket is None:
            raise RuntimeError("Sender is not connected. Use 'with' or call connect() first.")
        self._socket.send_string(json.dumps(payload, ensure_ascii=False))

    def send_protocols(self, protocols: Sequence[Dict[str, Any]]) -> None:
        self.send_payload({"protocols": list(protocols)})

    def send_protocol(
        self,
        operations: Sequence[Dict[str, Any]],
        *,
        repetitions: int = 1,
        infinite_repetition: bool = False,
    ) -> None:
        protocol = self.make_protocol(
            operations,
            repetitions=repetitions,
            infinite_repetition=infinite_repetition,
        )
        self.send_protocols([protocol])

    def send_operations(
        self,
        operations: Sequence[Dict[str, Any]],
        *,
        repetitions: int = 1,
        infinite_repetition: bool = False,
    ) -> None:
        self.send_protocol(
            operations,
            repetitions=repetitions,
            infinite_repetition=infinite_repetition,
        )

    def send_json_file(self, json_file: Union[str, Path]) -> None:
        payload = self.load_payload_from_file(json_file)
        self.send_payload(payload)

    @staticmethod
    def make_operation(duration_in_ms: int, eio_states: Iterable[Union[bool, int]]) -> Dict[str, Any]:
        states = [bool(v) for v in eio_states]
        if len(states) != 8:
            raise ValueError(f"eio_states must contain exactly 8 values, got {len(states)}")
        return {
            "duration_in_ms": int(duration_in_ms),
            "eioStates": states,
        }

    @staticmethod
    def make_protocol(
        operations: Sequence[Dict[str, Any]],
        *,
        repetitions: int = 1,
        infinite_repetition: bool = False,
    ) -> Dict[str, Any]:
        if repetitions < 1 and not infinite_repetition:
            raise ValueError("repetitions must be >= 1 when infinite_repetition is False")
        return {
            "repetitions": int(repetitions),
            "infinite_repetition": bool(infinite_repetition),
            "operations": list(operations),
        }

    @staticmethod
    def load_payload_from_file(json_file: Union[str, Path]) -> JsonPayload:
        path = Path(json_file)
        with path.open("r", encoding="utf-8") as f:
            payload = json.load(f)
        if not isinstance(payload, (dict, list)):
            raise ValueError("JSON payload root must be object or array")
        return payload


def send_once(
    operations: Sequence[Dict[str, Any]],
    *,
    host: str = "127.0.0.1",
    port: int = 33090,
    repetitions: int = 1,
    infinite_repetition: bool = False,
    connect_wait_ms: int = 100,
    linger_ms: int = 2000,
    send_timeout_ms: int = 2000,
) -> None:
    """Convenience helper for one-shot sending without manual lifecycle handling."""
    with LabJackProtocolSender(
        host=host,
        port=port,
        connect_wait_ms=connect_wait_ms,
        linger_ms=linger_ms,
        send_timeout_ms=send_timeout_ms,
    ) as sender:
        sender.send_operations(
            operations,
            repetitions=repetitions,
            infinite_repetition=infinite_repetition,
        )


if __name__ == "__main__":
    with LabJackProtocolSender(connect_wait_ms=300, linger_ms=3000) as sender:
        ops = [
            sender.make_operation(3000, [1, 0, 1, 0, 0, 0, 0, 0]),
            sender.make_operation(3000, [0, 1, 0, 1, 0, 0, 0, 0]),
        ]
        sender.send_operations(ops, repetitions=1)
        print(f"Command sent to {sender.endpoint}")
