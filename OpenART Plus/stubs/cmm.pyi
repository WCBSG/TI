"""
OpenART CMM（Configured Module Map）— 引脚映射管理。

通过 cmm_cfg.csv 定义引脚别名，代码中使用 cmm 管理的引脚名。
"""

from typing import Dict, Tuple, Optional

def add(mapping: Dict[str, Tuple[str, str, object, Optional[object]]]) -> None:
    """
    注册引脚映射字典。

    key: "fn.unit.signal" 或 "fn.signal" 格式
    value: (hint, pinobj_string, pin_object, owner)
    """
    ...

def get(name: str) -> object:
    """按名称获取已注册的引脚对象。"""
    ...

def list() -> list:
    """列出所有已注册的引脚名。"""
    ...
