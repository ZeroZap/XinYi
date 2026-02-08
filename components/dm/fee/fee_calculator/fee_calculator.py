#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
FEE (Flash EEPROM Emulation) 配置计算工具 v2.1
修正版：强制 write_granularity >= 8
"""

import tkinter as tk
from tkinter import ttk, messagebox, filedialog
import json
import math
import os
from datetime import datetime

# FEE设计约束
FEE_MIN_GRANULARITY = 8
FEE_MAX_GRANULARITY = 256
FEE_RECORD_HEADER_SIZE = 4

class MCUDatabase:
    """MCU配置数据库管理"""

    def __init__(self, db_file='mcu_database.json'):
        self.db_file = db_file
        self.data = None
        self.load()

    def load(self):
        """加载数据库"""
        try:
            if os.path.exists(self.db_file):
                with open(self.db_file, 'r', encoding='utf-8') as f:
                    self.data = json.load(f)
                # 验证数据库
                self.validate_database()
            else:
                self.data = self.create_default_db()
                self.save()
        except Exception as e:
            messagebox.showerror("数据库错误", f"加载MCU数据库失败: {str(e)}")
            self.data = self.create_default_db()

    def validate_database(self):
        """验证数据库中的MCU配置"""
        invalid_mcus = []
        for mcu in self.data.get('mcus', []):
            gran = mcu.get('fee_write_granularity', 0)
            if gran < FEE_MIN_GRANULARITY:
                invalid_mcus.append(f"{mcu['name']}: 颗粒度{gran}B < {FEE_MIN_GRANULARITY}B")

        if invalid_mcus:
            msg = "警告：数据库中存在不符合FEE设计的配置：\n\n"
            msg += "\n".join(invalid_mcus)
            msg += f"\n\nFEE要求 write_granularity >= {FEE_MIN_GRANULARITY}字节"
            messagebox.showwarning("数据库验证", msg)

    def save(self):
        """保存数据库"""
        try:
            self.data['last_updated'] = datetime.now().strftime('%Y-%m-%d')
            with open(self.db_file, 'w', encoding='utf-8') as f:
                json.dump(self.data, f, indent=2, ensure_ascii=False)
            return True
        except Exception as e:
            messagebox.showerror("保存失败", f"保存MCU数据库失败: {str(e)}")
            return False

    def create_default_db(self):
        """创建默认数据库"""
        return {
            "version": "2.0",
            "last_updated": datetime.now().strftime('%Y-%m-%d'),
            "mcus": [],
            "manufacturers": [],
            "granularity_options": [8, 16, 32, 64, 128, 256],
            "design_constraints": {
                "min_granularity": FEE_MIN_GRANULARITY,
                "max_granularity": FEE_MAX_GRANULARITY,
                "record_header_size": FEE_RECORD_HEADER_SIZE,
                "reason": f"Record = Header({FEE_RECORD_HEADER_SIZE}B) + Data(gran-{FEE_RECORD_HEADER_SIZE}B)，确保至少4字节有效数据"
            }
        }

    def get_all_mcus(self):
        """获取所有MCU列表"""
        return self.data.get('mcus', [])

    def get_mcu_by_id(self, mcu_id):
        """根据ID获取MCU配置"""
        for mcu in self.data.get('mcus', []):
            if mcu['id'] == mcu_id:
                return mcu
        return None

    def get_mcu_by_name(self, name):
        """根据名称获取MCU配置"""
        for mcu in self.data.get('mcus', []):
            if mcu['name'] == name:
                return mcu
        return None

    def add_mcu(self, mcu_config):
        """添加新MCU配置"""
        # 验证颗粒度
        gran = mcu_config.get('fee_write_granularity', 0)
        if gran < FEE_MIN_GRANULARITY:
            messagebox.showerror("验证失败",
                f"FEE写入颗粒度必须 >= {FEE_MIN_GRANULARITY}字节\n"
                f"当前值：{gran}字节")
            return False

        if 'mcus' not in self.data:
            self.data['mcus'] = []
        self.data['mcus'].append(mcu_config)
        return self.save()

    def update_mcu(self, mcu_id, mcu_config):
        """更新MCU配置"""
        # 验证颗粒度
        gran = mcu_config.get('fee_write_granularity', 0)
        if gran < FEE_MIN_GRANULARITY:
            messagebox.showerror("验证失败",
                f"FEE写入颗粒度必须 >= {FEE_MIN_GRANULARITY}字节\n"
                f"当前值：{gran}字节")
            return False

        for i, mcu in enumerate(self.data.get('mcus', [])):
            if mcu['id'] == mcu_id:
                self.data['mcus'][i] = mcu_config
                return self.save()
        return False

    def delete_mcu(self, mcu_id):
        """删除MCU配置"""
        self.data['mcus'] = [m for m in self.data.get('mcus', [])
                            if m['id'] != mcu_id]
        return self.save()

    def get_manufacturers(self):
        """获取制造商列表"""
        return sorted(set(self.data.get('manufacturers', [])))

    def get_granularity_options(self):
        """获取颗粒度选项"""
        options = self.data.get('granularity_options', [8, 16, 32, 64, 128, 256])
        # 过滤掉小于最小值的选项
        return [g for g in options if g >= FEE_MIN_GRANULARITY]

class MCUEditorDialog(tk.Toplevel):
    """MCU配置编辑对话框"""

    def __init__(self, parent, db, mcu_config=None):
        super().__init__(parent)
        self.db = db
        self.mcu_config = mcu_config
        self.result = None

        self.title("编辑MCU配置" if mcu_config else "添加MCU配置")
        self.geometry("550x700")
        self.resizable(False, False)

        self.create_widgets()

        if mcu_config:
            self.load_config(mcu_config)

        self.transient(parent)
        self.grab_set()

    def create_widgets(self):
        """创建界面"""
        frame = ttk.Frame(self, padding="10")
        frame.pack(fill=tk.BOTH, expand=True)

        row = 0

        # 设计约束提示
        constraint_frame = ttk.LabelFrame(frame, text="⚠️ FEE设计约束", padding="5")
        constraint_frame.grid(row=row, column=0, columnspan=2, sticky=(tk.W, tk.E), pady=5)

        constraint_text = (
            f"• FEE写入颗粒度必须 >= {FEE_MIN_GRANULARITY} 字节\n"
            f"• Record = Header({FEE_RECORD_HEADER_SIZE}B) + Data(gran-{FEE_RECORD_HEADER_SIZE}B)\n"
            f"• 确保至少{FEE_MIN_GRANULARITY-FEE_RECORD_HEADER_SIZE}字节有效数据"
        )
        ttk.Label(constraint_frame, text=constraint_text, foreground="red").pack(anchor=tk.W)
        row += 1

        # ID
        ttk.Label(frame, text="ID (唯一标识):").grid(row=row, column=0, sticky=tk.W, pady=5)
        self.id_var = tk.StringVar()
        ttk.Entry(frame, textvariable=self.id_var, width=35).grid(row=row, column=1, sticky=tk.W, pady=5)
        row += 1

        # 名称
        ttk.Label(frame, text="名称:").grid(row=row, column=0, sticky=tk.W, pady=5)
        self.name_var = tk.StringVar()
        ttk.Entry(frame, textvariable=self.name_var, width=35).grid(row=row, column=1, sticky=tk.W, pady=5)
        row += 1

        # 制造商
        ttk.Label(frame, text="制造商:").grid(row=row, column=0, sticky=tk.W, pady=5)
        self.manufacturer_var = tk.StringVar()
        manu_combo = ttk.Combobox(frame, textvariable=self.manufacturer_var, width=32)
        manu_combo['values'] = self.db.get_manufacturers()
        manu_combo.grid(row=row, column=1, sticky=tk.W, pady=5)
        row += 1

        # 系列
        ttk.Label(frame, text="系列:").grid(row=row, column=0, sticky=tk.W, pady=5)
        self.series_var = tk.StringVar()
        ttk.Entry(frame, textvariable=self.series_var, width=35).grid(row=row, column=1, sticky=tk.W, pady=5)
        row += 1

        # Flash Page大小
        ttk.Label(frame, text="Flash Page大小:").grid(row=row, column=0, sticky=tk.W, pady=5)
        page_frame = ttk.Frame(frame)
        page_frame.grid(row=row, column=1, sticky=tk.W, pady=5)
        self.flash_page_var = tk.IntVar(value=2048)
        ttk.Entry(page_frame, textvariable=self.flash_page_var, width=20).pack(side=tk.LEFT)
        ttk.Label(page_frame, text="字节").pack(side=tk.LEFT, padx=5)
        row += 1

        # Flash写入颗粒度（原生）
        ttk.Label(frame, text="Flash写入颗粒度:").grid(row=row, column=0, sticky=tk.W, pady=5)
        flash_gran_frame = ttk.Frame(frame)
        flash_gran_frame.grid(row=row, column=1, sticky=tk.W, pady=5)
        self.flash_gran_var = tk.IntVar(value=8)
        ttk.Entry(flash_gran_frame, textvariable=self.flash_gran_var, width=20).pack(side=tk.LEFT)
        ttk.Label(flash_gran_frame, text="字节 (原生)").pack(side=tk.LEFT, padx=5)
        row += 1

        # FEE写入颗粒度（推荐值）
        ttk.Label(frame, text="FEE写入颗粒度:").grid(row=row, column=0, sticky=tk.W, pady=5)
        fee_gran_frame = ttk.Frame(frame)
        fee_gran_frame.grid(row=row, column=1, sticky=tk.W, pady=5)
        self.fee_granularity_var = tk.IntVar(value=8)
        gran_combo = ttk.Combobox(fee_gran_frame, textvariable=self.fee_granularity_var, width=17)
        gran_combo['values'] = self.db.get_granularity_options()
        gran_combo.pack(side=tk.LEFT)
        ttk.Label(fee_gran_frame, text="字节 ⚠️").pack(side=tk.LEFT, padx=5)
        row += 1

        # 提示
        hint_label = ttk.Label(frame, text=f"(必须 >= {FEE_MIN_GRANULARITY})", foreground="red", font=('TkDefaultFont', 8))
        hint_label.grid(row=row, column=1, sticky=tk.W, pady=0)
        row += 1

        # 最小写入大小
        ttk.Label(frame, text="最小写入大小:").grid(row=row, column=0, sticky=tk.W, pady=5)
        min_frame = ttk.Frame(frame)
        min_frame.grid(row=row, column=1, sticky=tk.W, pady=5)
        self.min_write_var = tk.IntVar(value=8)
        ttk.Entry(min_frame, textvariable=self.min_write_var, width=20).pack(side=tk.LEFT)
        ttk.Label(min_frame, text="字节").pack(side=tk.LEFT, padx=5)
        row += 1

        # 最大擦除次数
        ttk.Label(frame, text="最大擦除次数:").grid(row=row, column=0, sticky=tk.W, pady=5)
        erase_frame = ttk.Frame(frame)
        erase_frame.grid(row=row, column=1, sticky=tk.W, pady=5)
        self.max_erase_var = tk.IntVar(value=10000)
        ttk.Entry(erase_frame, textvariable=self.max_erase_var, width=20).pack(side=tk.LEFT)
        ttk.Label(erase_frame, text="次").pack(side=tk.LEFT, padx=5)
        row += 1

        # 推荐Pages
        ttk.Label(frame, text="推荐Pages/FEE:").grid(row=row, column=0, sticky=tk.W, pady=5)
        self.rec_pages_var = tk.IntVar(value=2)
        ttk.Spinbox(frame, from_=1, to=8, textvariable=self.rec_pages_var, width=33).grid(row=row, column=1, sticky=tk.W, pady=5)
        row += 1

        # 备注
        ttk.Label(frame, text="备注:").grid(row=row, column=0, sticky=tk.W, pady=5)
        self.notes_var = tk.StringVar()
        notes_entry = ttk.Entry(frame, textvariable=self.notes_var, width=35)
        notes_entry.grid(row=row, column=1, sticky=tk.W, pady=5)
        row += 1

        # 数据手册URL
        ttk.Label(frame, text="数据手册URL:").grid(row=row, column=0, sticky=tk.W, pady=5)
        self.datasheet_var = tk.StringVar()
        ttk.Entry(frame, textvariable=self.datasheet_var, width=35).grid(row=row, column=1, sticky=tk.W, pady=5)
        row += 1

        # 按钮
        button_frame = ttk.Frame(frame)
        button_frame.grid(row=row, column=0, columnspan=2, pady=20)
        ttk.Button(button_frame, text="保存", command=self.save).pack(side=tk.LEFT, padx=5)
        ttk.Button(button_frame, text="取消", command=self.cancel).pack(side=tk.LEFT, padx=5)

    def load_config(self, config):
        """加载配置到界面"""
        self.id_var.set(config.get('id', ''))
        self.name_var.set(config.get('name', ''))
        self.manufacturer_var.set(config.get('manufacturer', ''))
        self.series_var.set(config.get('series', ''))
        self.flash_page_var.set(config.get('flash_page_size', 2048))
        self.flash_gran_var.set(config.get('flash_write_granularity', 8))
        self.fee_granularity_var.set(config.get('fee_write_granularity', 8))
        self.min_write_var.set(config.get('min_write_size', 8))
        self.max_erase_var.set(config.get('max_erase_cycles', 10000))
        self.rec_pages_var.set(config.get('recommended_pages_per_fee', 2))
        self.notes_var.set(config.get('notes', ''))
        self.datasheet_var.set(config.get('datasheet_url', ''))

    def save(self):
        """保存配置"""
        # 验证
        if not self.id_var.get():
            messagebox.showerror("错误", "ID不能为空")
            return

        if not self.name_var.get():
            messagebox.showerror("错误", "名称不能为空")
            return

        # 验证FEE写入颗粒度
        fee_gran = self.fee_granularity_var.get()
        if fee_gran < FEE_MIN_GRANULARITY:
            messagebox.showerror("验证失败",
                f"FEE写入颗粒度必须 >= {FEE_MIN_GRANULARITY}字节\n"
                f"当前值：{fee_gran}字节\n\n"
                f"原因：Record = Header({FEE_RECORD_HEADER_SIZE}B) + Data({fee_gran-FEE_RECORD_HEADER_SIZE}B)\n"
                f"需要至少{FEE_MIN_GRANULARITY-FEE_RECORD_HEADER_SIZE}字节有效数据")
            return

        # 检查ID是否重复（新增时）
        if not self.mcu_config:
            if self.db.get_mcu_by_id(self.id_var.get()):
                messagebox.showerror("错误", "ID已存在，请使用其他ID")
                return

        # 构造配置
        self.result = {
            'id': self.id_var.get(),
            'name': self.name_var.get(),
            'manufacturer': self.manufacturer_var.get(),
            'series': self.series_var.get(),
            'flash_page_size': self.flash_page_var.get(),
            'flash_write_granularity': self.flash_gran_var.get(),
            'fee_write_granularity': fee_gran,
            'min_write_size': self.min_write_var.get(),
            'max_erase_cycles': self.max_erase_var.get(),
            'recommended_pages_per_fee': self.rec_pages_var.get(),
            'notes': self.notes_var.get(),
            'datasheet_url': self.datasheet_var.get()
        }

        self.destroy()

    def cancel(self):
        """取消"""
        self.result = None
        self.destroy()

class FEECalculator:
    def __init__(self, root):
        self.root = root
        self.root.title(f"FEE 配置计算工具 v2.1 (颗粒度 >= {FEE_MIN_GRANULARITY}字节)")
        self.root.geometry("900x800")

        # 加载MCU数据库
        self.db = MCUDatabase()

        # 设置样式
        style = ttk.Style()
        style.theme_use('clam')

        self.create_menu()
        self.create_widgets()

    def create_menu(self):
        """创建菜单栏"""
        menubar = tk.Menu(self.root)
        self.root.config(menu=menubar)

        # 文件菜单
        file_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="文件", menu=file_menu)
        file_menu.add_command(label="导出配置", command=self.export_config)
        file_menu.add_command(label="导入配置", command=self.import_config)
        file_menu.add_separator()
        file_menu.add_command(label="退出", command=self.root.quit)

        # MCU数据库菜单
        db_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="MCU数据库", menu=db_menu)
        db_menu.add_command(label="管理MCU配置", command=self.manage_mcu_database)
        db_menu.add_command(label="刷新数据库", command=self.refresh_database)
        db_menu.add_command(label="验证数据库", command=self.validate_database)
        db_menu.add_separator()
        db_menu.add_command(label="导出数据库", command=self.export_database)
        db_menu.add_command(label="导入数据库", command=self.import_database)

        # 帮助菜单
        help_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="帮助", menu=help_menu)
        help_menu.add_command(label="FEE设计约束", command=self.show_design_constraints)
        help_menu.add_command(label="Flash颗粒度转换",
                     command=self.show_flash_granularity_info)
        help_menu.add_command(label="使用说明", command=self.show_help)
        help_menu.add_command(label="关于", command=self.show_about)

    def create_widgets(self):
        # 主框架
        main_frame = ttk.Frame(self.root, padding="10")
        main_frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))

        # ============ 设计约束提示 ============
        constraint_frame = ttk.LabelFrame(main_frame, text="⚠️ FEE设计约束", padding="5")
        constraint_frame.grid(row=0, column=0, sticky=(tk.W, tk.E), pady=(0, 10))

        constraint_text = (
            f"写入颗粒度必须 >= {FEE_MIN_GRANULARITY}字节 | "
            f"Record = Header({FEE_RECORD_HEADER_SIZE}B) + Data(gran-{FEE_RECORD_HEADER_SIZE}B) | "
            f"确保至少{FEE_MIN_GRANULARITY-FEE_RECORD_HEADER_SIZE}字节有效数据"
        )
        ttk.Label(constraint_frame, text=constraint_text, foreground="red",
                 font=('TkDefaultFont', 9, 'bold')).pack()

        # ============ 输入区域 ============
        input_frame = ttk.LabelFrame(main_frame, text="输入参数", padding="10")
        input_frame.grid(row=1, column=0, sticky=(tk.W, tk.E), pady=5)

        row = 0

        # MCU型号选择
        mcu_select_frame = ttk.Frame(input_frame)
        mcu_select_frame.grid(row=row, column=0, columnspan=2, sticky=tk.W, pady=5)

        ttk.Label(mcu_select_frame, text="MCU型号:").pack(side=tk.LEFT)
        self.mcu_var = tk.StringVar(value="自定义")
        self.mcu_combo = ttk.Combobox(mcu_select_frame, textvariable=self.mcu_var, width=35)
        self.update_mcu_list()
        self.mcu_combo.pack(side=tk.LEFT, padx=5)
        self.mcu_combo.bind('<<ComboboxSelected>>', self.on_mcu_selected)

        ttk.Button(mcu_select_frame, text="ℹ", width=3,
                  command=self.show_mcu_info).pack(side=tk.LEFT, padx=2)
        row += 1

        # Flash Page 大小
        ttk.Label(input_frame, text="Flash Page大小:").grid(row=row, column=0, sticky=tk.W, pady=5)
        flash_page_frame = ttk.Frame(input_frame)
        flash_page_frame.grid(row=row, column=1, sticky=tk.W, pady=5)
        self.flash_page_size = tk.IntVar(value=2048)
        ttk.Entry(flash_page_frame, textvariable=self.flash_page_size, width=15).pack(side=tk.LEFT)
        ttk.Label(flash_page_frame, text="字节").pack(side=tk.LEFT, padx=5)
        row += 1

        # 写入颗粒度
        ttk.Label(input_frame, text="FEE写入颗粒度:").grid(row=row, column=0, sticky=tk.W, pady=5)
        gran_frame = ttk.Frame(input_frame)
        gran_frame.grid(row=row, column=1, sticky=tk.W, pady=5)
        self.granularity = tk.IntVar(value=8)
        gran_combo = ttk.Combobox(gran_frame, textvariable=self.granularity, width=12)
        gran_combo['values'] = self.db.get_granularity_options()
        gran_combo.pack(side=tk.LEFT)
        ttk.Label(gran_frame, text=f"字节 (>={FEE_MIN_GRANULARITY})",
                 foreground="red").pack(side=tk.LEFT, padx=5)
        row += 1

        # Cache大小
        ttk.Label(input_frame, text="虚拟EEPROM大小:").grid(row=row, column=0, sticky=tk.W, pady=5)
        cache_frame = ttk.Frame(input_frame)
        cache_frame.grid(row=row, column=1, sticky=tk.W, pady=5)
        self.cache_size = tk.IntVar(value=512)
        ttk.Entry(cache_frame, textvariable=self.cache_size, width=15).pack(side=tk.LEFT)
        ttk.Label(cache_frame, text="字节").pack(side=tk.LEFT, padx=5)
        row += 1

        # 每个FEE Page的Flash Page数
        ttk.Label(input_frame, text="每个FEE Page包含:").grid(row=row, column=0, sticky=tk.W, pady=5)
        pages_frame = ttk.Frame(input_frame)
        pages_frame.grid(row=row, column=1, sticky=tk.W, pady=5)
        self.pages_per_fee = tk.IntVar(value=2)
        ttk.Spinbox(pages_frame, from_=1, to=8, textvariable=self.pages_per_fee, width=13).pack(side=tk.LEFT)
        ttk.Label(pages_frame, text="个Flash Page").pack(side=tk.LEFT, padx=5)
        row += 1

        # 分隔线
        ttk.Separator(input_frame, orient='horizontal').grid(row=row, column=0, columnspan=2, sticky=(tk.W, tk.E), pady=10)
        row += 1

        # 应用场景参数
        ttk.Label(input_frame, text="预期日均写入次数:").grid(row=row, column=0, sticky=tk.W, pady=5)
        writes_frame = ttk.Frame(input_frame)
        writes_frame.grid(row=row, column=1, sticky=tk.W, pady=5)
        self.daily_writes = tk.IntVar(value=1000)
        ttk.Entry(writes_frame, textvariable=self.daily_writes, width=15).pack(side=tk.LEFT)
        ttk.Label(writes_frame, text="次/天").pack(side=tk.LEFT, padx=5)
        row += 1

        # Flash擦除寿命
        ttk.Label(input_frame, text="Flash擦除寿命:").grid(row=row, column=0, sticky=tk.W, pady=5)
        erase_frame = ttk.Frame(input_frame)
        erase_frame.grid(row=row, column=1, sticky=tk.W, pady=5)
        self.max_erase = tk.IntVar(value=10000)
        ttk.Entry(erase_frame, textvariable=self.max_erase, width=15).pack(side=tk.LEFT)
        ttk.Label(erase_frame, text="次").pack(side=tk.LEFT, padx=5)
        row += 1

        # 计算按钮
        ttk.Button(input_frame, text="计算", command=self.calculate,
                  style='Accent.TButton').grid(row=row, column=0, columnspan=2, pady=15)

        # ============ 输出区域 ============
        output_frame = ttk.LabelFrame(main_frame, text="计算结果", padding="10")
        output_frame.grid(row=2, column=0, sticky=(tk.W, tk.E), pady=5)

        # 结果文本框
        self.result_text = tk.Text(output_frame, height=15, width=100,
                                   font=('Consolas', 9), state='disabled')
        self.result_text.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

        # 滚动条
        scrollbar = ttk.Scrollbar(output_frame, command=self.result_text.yview)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        self.result_text['yscrollcommand'] = scrollbar.set

        # ============ 代码生成区域 ============
        code_frame = ttk.LabelFrame(main_frame, text="配置代码", padding="10")
        code_frame.grid(row=3, column=0, sticky=(tk.W, tk.E), pady=5)

        button_frame = ttk.Frame(code_frame)
        button_frame.pack(fill=tk.X, pady=5)

        ttk.Button(button_frame, text="生成C代码",
                  command=self.generate_c_code).pack(side=tk.LEFT, padx=5)
        ttk.Button(button_frame, text="复制代码",
                  command=self.copy_to_clipboard).pack(side=tk.LEFT, padx=5)
        ttk.Button(button_frame, text="保存代码",
                  command=self.save_code).pack(side=tk.LEFT, padx=5)

        self.code_text = tk.Text(code_frame, height=8, width=100,
                                font=('Consolas', 9), state='disabled')
        self.code_text.pack(fill=tk.BOTH, expand=True)

    def update_mcu_list(self):
        """更新MCU列表"""
        mcus = self.db.get_all_mcus()
        mcu_names = []
        for m in mcus:
            gran = m.get('fee_write_granularity', 0)
            if gran >= FEE_MIN_GRANULARITY:
                mcu_names.append(f"{m['name']} - {m['manufacturer']} (FEE:{gran}B)")
            else:
                mcu_names.append(f"{m['name']} - {m['manufacturer']} (⚠️gran:{gran}B)")
        mcu_names.append("自定义")
        self.mcu_combo['values'] = mcu_names

    def on_mcu_selected(self, event):
        """MCU选择事件处理"""
        selection = self.mcu_var.get()

        if selection == "自定义":
            return

        mcu_name = selection.split(' - ')[0]
        mcu = self.db.get_mcu_by_name(mcu_name)

        if mcu:
            self.flash_page_size.set(mcu['flash_page_size'])
            gran = mcu.get('fee_write_granularity', 8)

            # 验证颗粒度
            if gran < FEE_MIN_GRANULARITY:
                messagebox.showwarning("注意",
                    f"该MCU的FEE颗粒度({gran}B)小于最小要求({FEE_MIN_GRANULARITY}B)\n"
                    f"已自动调整为{FEE_MIN_GRANULARITY}字节")
                gran = FEE_MIN_GRANULARITY

            self.granularity.set(gran)
            self.max_erase.set(mcu['max_erase_cycles'])
            self.pages_per_fee.set(mcu['recommended_pages_per_fee'])

    def show_mcu_info(self):
        """显示MCU详细信息"""
        selection = self.mcu_var.get()

        if selection == "自定义":
            messagebox.showinfo("提示", "请先选择具体的MCU型号")
            return

        mcu_name = selection.split(' - ')[0]
        mcu = self.db.get_mcu_by_name(mcu_name)

        if mcu:
            flash_gran = mcu.get('flash_write_granularity', 'N/A')
            fee_gran = mcu.get('fee_write_granularity', 'N/A')

            info = f"""
MCU型号: {mcu['name']}
制造商: {mcu['manufacturer']}
系列: {mcu['series']}

Flash配置:
  Page大小: {mcu['flash_page_size']:,} 字节
  Flash原生写入颗粒: {flash_gran} 字节
  FEE写入颗粒: {fee_gran} 字节
  最小写入: {mcu['min_write_size']} 字节
  擦除寿命: {mcu['max_erase_cycles']:,} 次

FEE配置:
  推荐Pages/FEE: {mcu['recommended_pages_per_fee']}
  Record数据大小: {fee_gran - FEE_RECORD_HEADER_SIZE if isinstance(fee_gran, int) else 'N/A'} 字节

备注: {mcu.get('notes', '无')}
            """

            if mcu.get('datasheet_url'):
                info += f"\n数据手册: {mcu['datasheet_url']}"

            # 验证警告
            if isinstance(fee_gran, int) and fee_gran < FEE_MIN_GRANULARITY:
                info += f"\n\n⚠️ 警告: FEE颗粒度({fee_gran}B)小于最小要求({FEE_MIN_GRANULARITY}B)"

            messagebox.showinfo("MCU信息", info.strip())

    def manage_mcu_database(self):
        """管理MCU数据库"""
        db_window = tk.Toplevel(self.root)
        db_window.title("MCU数据库管理")
        db_window.geometry("800x550")

        frame = ttk.Frame(db_window, padding="10")
        frame.pack(fill=tk.BOTH, expand=True)

        ttk.Label(frame, text="已保存的MCU配置:", font=('TkDefaultFont', 10, 'bold')).pack(anchor=tk.W)

        list_frame = ttk.Frame(frame)
        list_frame.pack(fill=tk.BOTH, expand=True, pady=5)

        scrollbar = ttk.Scrollbar(list_frame)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

        listbox = tk.Listbox(list_frame, yscrollcommand=scrollbar.set, height=20, font=('Consolas', 9))
        listbox.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.config(command=listbox.yview)

        # 填充列表
        mcus = self.db.get_all_mcus()
        for mcu in mcus:
            fee_gran = mcu.get('fee_write_granularity', 0)
            warning = "⚠️" if fee_gran < FEE_MIN_GRANULARITY else "✓"
            line = f"{warning} {mcu['name']:<15} {mcu['manufacturer']:<20} FEE:{fee_gran}B"
            listbox.insert(tk.END, line)

        # 按钮
        button_frame = ttk.Frame(frame)
        button_frame.pack(fill=tk.X, pady=5)

        def add_mcu():
            dialog = MCUEditorDialog(db_window, self.db)
            db_window.wait_window(dialog)
            if dialog.result:
                if self.db.add_mcu(dialog.result):
                    self.refresh_database()
                    db_window.destroy()
                    self.manage_mcu_database()

        def edit_mcu():
            selection = listbox.curselection()
            if not selection:
                messagebox.showwarning("提示", "请先选择要编辑的MCU")
                return

            mcu = mcus[selection[0]]
            dialog = MCUEditorDialog(db_window, self.db, mcu)
            db_window.wait_window(dialog)
            if dialog.result:
                if self.db.update_mcu(mcu['id'], dialog.result):
                    self.refresh_database()
                    db_window.destroy()
                    self.manage_mcu_database()

        def delete_mcu():
            selection = listbox.curselection()
            if not selection:
                messagebox.showwarning("提示", "请先选择要删除的MCU")
                return

            mcu = mcus[selection[0]]
            if messagebox.askyesno("确认", f"确定要删除 {mcu['name']} 吗？"):
                self.db.delete_mcu(mcu['id'])
                self.refresh_database()
                db_window.destroy()
                self.manage_mcu_database()

        ttk.Button(button_frame, text="添加", command=add_mcu).pack(side=tk.LEFT, padx=5)
        ttk.Button(button_frame, text="编辑", command=edit_mcu).pack(side=tk.LEFT, padx=5)
        ttk.Button(button_frame, text="删除", command=delete_mcu).pack(side=tk.LEFT, padx=5)
        ttk.Button(button_frame, text="关闭", command=db_window.destroy).pack(side=tk.LEFT, padx=5)

    def refresh_database(self):
        """刷新数据库"""
        self.db.load()
        self.update_mcu_list()
        messagebox.showinfo("成功", "数据库已刷新")

    def validate_database(self):
        """验证数据库"""
        self.db.validate_database()

    def show_design_constraints(self):
        """显示设计约束"""
        info = f"""
FEE (Flash EEPROM Emulation) 设计约束
═══════════════════════════════════════

【核心约束】
写入颗粒度必须 >= {FEE_MIN_GRANULARITY} 字节

【原因分析】
Record结构：
  ┌──────────────────────┐
  │ Record Header (4B)   │
  │  ├─ addr (2B)        │
  │  └─ crc (2B)         │
  ├──────────────────────┤
  │ Data (gran - 4B)     │
  └──────────────────────┘

如果 gran < {FEE_MIN_GRANULARITY}:
  • gran=4: Data=0B ✗ 没有有效数据
  • gran=6: Data=2B ✗ 数据太少
  • gran=8: Data=4B ✓ 最小合理配置

【推荐配置】
  • 一般MCU: 8字节 (50%效率，4B数据)
  • 高性能:  16字节 (75%效率，12B数据)
  • 大容量:  32字节 (87.5%效率，28B数据)

【Flash原生颗粒度 vs FEE颗粒度】
很多MCU的Flash原生写入颗粒度小于8字节（如4字节）
但FEE层必须使用 >= {FEE_MIN_GRANULARITY}字节的对齐单位
以确保Record结构的完整性。

示例：STM32F103
  Flash原生: 4字节（半字写入）
  FEE使用:   8字节（软件层对齐）
  实现方式:  每次写入2个半字
        """
        messagebox.showinfo("FEE设计约束", info.strip())

    def export_database(self):
        """导出数据库"""
        filename = filedialog.asksaveasfilename(
            defaultextension=".json",
            filetypes=[("JSON文件", "*.json"), ("所有文件", "*.*")]
        )
        if filename:
            try:
                import shutil
                shutil.copy(self.db.db_file, filename)
                messagebox.showinfo("成功", f"数据库已导出到:\n{filename}")
            except Exception as e:
                messagebox.showerror("错误", f"导出失败: {str(e)}")

    def import_database(self):
        """导入数据库"""
        filename = filedialog.askopenfilename(
            filetypes=[("JSON文件", "*.json"), ("所有文件", "*.*")]
        )
        if filename:
            if messagebox.askyesno("确认", "导入将覆盖当前数据库，是否继续？"):
                try:
                    import shutil
                    shutil.copy(filename, self.db.db_file)
                    self.refresh_database()
                    messagebox.showinfo("成功", "数据库导入成功")
                except Exception as e:
                    messagebox.showerror("错误", f"导入失败: {str(e)}")

    def export_config(self):
        """导出当前配置"""
        config = {
            'mcu': self.mcu_var.get(),
            'flash_page_size': self.flash_page_size.get(),
            'granularity': self.granularity.get(),
            'cache_size': self.cache_size.get(),
            'pages_per_fee': self.pages_per_fee.get(),
            'daily_writes': self.daily_writes.get(),
            'max_erase': self.max_erase.get()
        }

        filename = filedialog.asksaveasfilename(
            defaultextension=".json",
            filetypes=[("JSON文件", "*.json"), ("所有文件", "*.*")]
        )
        if filename:
            try:
                with open(filename, 'w', encoding='utf-8') as f:
                    json.dump(config, f, indent=2, ensure_ascii=False)
                messagebox.showinfo("成功", f"配置已导出到:\n{filename}")
            except Exception as e:
                messagebox.showerror("错误", f"导出失败: {str(e)}")

    def import_config(self):
        """导入配置"""
        filename = filedialog.askopenfilename(
            filetypes=[("JSON文件", "*.json"), ("所有文件", "*.*")]
        )
        if filename:
            try:
                with open(filename, 'r', encoding='utf-8') as f:
                    config = json.load(f)

                self.mcu_var.set(config.get('mcu', '自定义'))
                self.flash_page_size.set(config.get('flash_page_size', 2048))

                gran = config.get('granularity', 8)
                if gran < FEE_MIN_GRANULARITY:
                    messagebox.showwarning("警告",
                        f"导入的颗粒度({gran}B)小于最小要求({FEE_MIN_GRANULARITY}B)\n"
                        f"已自动调整为{FEE_MIN_GRANULARITY}字节")
                    gran = FEE_MIN_GRANULARITY

                self.granularity.set(gran)
                self.cache_size.set(config.get('cache_size', 512))
                self.pages_per_fee.set(config.get('pages_per_fee', 2))
                self.daily_writes.set(config.get('daily_writes', 1000))
                self.max_erase.set(config.get('max_erase', 10000))

                messagebox.showinfo("成功", "配置导入成功")
            except Exception as e:
                messagebox.showerror("错误", f"导入失败: {str(e)}")
    def show_flash_granularity_info(self):
        """显示Flash颗粒度转换说明"""
        info = """
Flash原生颗粒度 vs FEE颗粒度转换说明
═══════════════════════════════════════════════

【原理】
FEE层使用更大的对齐单位（>=8字节）
底层通过Flash适配器自动拆分为原生写入

【示例：STM32F103】
┌────────────────────────────────┐
│ FEE写入: 8字节                  │
└───────────┬────────────────────┘
            │
┌───────────▼────────────────────┐
│ 适配器拆分                      │
│  ├─ 第1次: 4字节 (Word 0-3)    │
│  └─ 第2次: 4字节 (Word 4-7)    │
└───────────┬────────────────────┘
            │
┌───────────▼────────────────────┐
│ HAL_FLASH_Program()            │
│ (硬件4字节写入)                 │
└────────────────────────────────┘

【性能开销】
• gran=8:  2次写入 (~106μs)
• gran=16: 4次写入 (~212μs)
• 开销可接受，不影响整体性能

【实现位置】
在flash_ops->write()函数中实现转换
对FEE层完全透明

【支持的MCU】
✓ STM32F1/F4 (4B→8B/16B)
✓ ESP32 (4B→8B)
✓ nRF52 (4B→8B)
✓ 所有原生写入<8B的MCU
    """
        messagebox.showinfo("Flash颗粒度转换", info.strip())

    def show_help(self):
        """显示帮助"""
        help_text = f"""
FEE配置计算工具使用说明 v2.1

【重要】设计约束:
  写入颗粒度必须 >= {FEE_MIN_GRANULARITY}字节
  原因: Record = Header({FEE_RECORD_HEADER_SIZE}B) + Data(gran-{FEE_RECORD_HEADER_SIZE}B)
  需要至少{FEE_MIN_GRANULARITY-FEE_RECORD_HEADER_SIZE}字节有效数据

【使用步骤】
1. 选择MCU型号（或选择"自定义"）
2. 调整参数（Cache大小、日写入次数等）
3. 点击"计算"按钮
4. 查看计算结果和建议
5. 生成C代码并复制使用

【MCU数据库管理】
• 菜单 -> MCU数据库 -> 管理MCU配置
• 可添加、编辑、删除MCU配置
• 支持导入/导出数据库
• 数据库验证功能

【注意事项】
• Flash原生颗粒度可能小于FEE颗粒度
• STM32F103: Flash 4B, FEE 8B (软件对齐)
• 添加MCU时会自动验证颗粒度
        """
        messagebox.showinfo("使用说明", help_text.strip())

    def show_about(self):
        """关于"""
        about_text = f"""
FEE配置计算工具 v2.1

Flash EEPROM Emulation配置计算和代码生成工具

特性:
✓ JSON数据库管理MCU配置
✓ 强制颗粒度 >= {FEE_MIN_GRANULARITY}字节验证
✓ 自动计算Flash占用和寿命
✓ 生成C配置代码
✓ 支持配置导入/导出
✓ 数据库验证功能

更新:
v2.1 - 强制FEE颗粒度 >= {FEE_MIN_GRANULARITY}字节
v2.0 - JSON数据库支持
v1.0 - 基础计算功能

© 2024
        """
        messagebox.showinfo("关于", about_text.strip())

    def calculate(self):
        """执行计算"""
        try:
            flash_page = self.flash_page_size.get()
            gran = self.granularity.get()
            cache = self.cache_size.get()
            pages_per = self.pages_per_fee.get()
            daily = self.daily_writes.get()
            max_erase = self.max_erase.get()

            # 验证颗粒度
            if gran < FEE_MIN_GRANULARITY:
                raise ValueError(
                    f"写入颗粒度必须 >= {FEE_MIN_GRANULARITY}字节\n"
                    f"当前值：{gran}字节\n\n"
                    f"原因：Record = Header({FEE_RECORD_HEADER_SIZE}B) + Data({gran-FEE_RECORD_HEADER_SIZE}B)\n"
                    f"需要至少{FEE_MIN_GRANULARITY-FEE_RECORD_HEADER_SIZE}字节有效数据"
                )

            if gran > FEE_MAX_GRANULARITY or (gran & (gran - 1)) != 0:
                raise ValueError(f"写入颗粒度必须是2的幂且 <= {FEE_MAX_GRANULARITY}")

            if cache < 64 or cache > 65536:
                raise ValueError("Cache大小应在64-65536字节之间")

            fee_page_size = flash_page * pages_per
            aligned_header = math.ceil(8 / gran) * gran
            data_area = fee_page_size - aligned_header
            record_size = gran
            record_data_size = gran - FEE_RECORD_HEADER_SIZE
            max_records = data_area // record_size

            handle_size = 28
            work_size = gran * 2
            total_ram = handle_size + cache + work_size

            avg_valid_records = (cache // record_data_size) // 2
            gc_trigger_records = max_records * 0.9
            writes_before_gc = (gc_trigger_records - avg_valid_records) // (cache / record_data_size)

            total_flash = fee_page_size * 2
            gc_per_year = (daily * 365) / writes_before_gc if writes_before_gc > 0 else 0
            lifespan_years = max_erase / gc_per_year if gc_per_year > 0 else float('inf')

            report = self.generate_report(
                flash_page, gran, cache, pages_per, daily, max_erase,
                fee_page_size, aligned_header, data_area, record_size,
                record_data_size, max_records, total_ram, handle_size,
                work_size, writes_before_gc, gc_per_year, lifespan_years, total_flash
            )

            self.result_text.config(state='normal')
            self.result_text.delete(1.0, tk.END)
            self.result_text.insert(1.0, report)
            self.result_text.config(state='disabled')

            self.generate_c_code()

        except ValueError as e:
            messagebox.showerror("参数错误", str(e))
        except Exception as e:
            messagebox.showerror("计算错误", f"计算过程出错: {str(e)}")

    def generate_report(self, flash_page, gran, cache, pages_per, daily, max_erase,
                       fee_page_size, aligned_header, data_area, record_size,
                       record_data_size, max_records, total_ram, handle_size,
                       work_size, writes_before_gc, gc_per_year, lifespan_years, total_flash):
        """生成计算报告"""
        report = []
        report.append("═" * 80)
        report.append("  FEE 配置计算结果")
        report.append("═" * 80)
        report.append("")

        # 设计验证
        report.append("【设计验证】")
        if gran >= FEE_MIN_GRANULARITY:
            report.append(f"  ✓ 写入颗粒度: {gran}字节 >= {FEE_MIN_GRANULARITY}字节 (符合要求)")
            report.append(f"  ✓ Record数据: {record_data_size}字节 (Header:{FEE_RECORD_HEADER_SIZE}B + Data:{record_data_size}B)")
        else:
            report.append(f"  ✗ 写入颗粒度: {gran}字节 < {FEE_MIN_GRANULARITY}字节 (不符合要求)")
            report.append(f"  ✗ Record数据: {record_data_size}字节 < 4字节 (数据不足)")
        report.append("")

        report.append("【基础参数】")
        mcu_name = self.mcu_var.get()
        if mcu_name != "自定义":
            report.append(f"  MCU型号:              {mcu_name}")
        report.append(f"  Flash Page大小:       {flash_page:,} 字节")
        report.append(f"  FEE写入颗粒度:        {gran} 字节")
        report.append(f"  虚拟EEPROM大小:       {cache:,} 字节")
        report.append(f"  每个FEE Page:         {pages_per} × Flash Page")
        report.append(f"  日均写入次数:         {daily:,} 次/天")
        report.append(f"  Flash擦除寿命:        {max_erase:,} 次")
        report.append("")

        report.append("【FEE配置】")
        report.append(f"  FEE Page数量:         2 个（固定双缓冲）")
        report.append(f"  单个FEE Page大小:     {fee_page_size:,} 字节 ({fee_page_size/1024:.1f} KB)")
        report.append(f"  总Flash占用:          {total_flash:,} 字节 ({total_flash/1024:.1f} KB)")
        report.append("")

        report.append("【存储布局】")
        report.append(f"  Page Header (对齐):   {aligned_header} 字节")
        if aligned_header > 8:
            report.append(f"    ├─ 基础大小:        8 字节")
            report.append(f"    └─ 对齐填充:        {aligned_header-8} 字节")
        report.append(f"  数据区大小:           {data_area:,} 字节")
        report.append(f"  Record结构:")
        report.append(f"    ├─ 总大小:          {record_size} 字节 (= 1个颗粒)")
        report.append(f"    ├─ Header:          {FEE_RECORD_HEADER_SIZE} 字节 (addr:2B + crc:2B)")
        report.append(f"    └─ Data:            {record_data_size} 字节")
        report.append(f"  最大Record数:         {max_records} 条")
        report.append(f"  最大数据容量:         {max_records * record_data_size:,} 字节")
        report.append("")

        report.append("【RAM占用】")
        report.append(f"  FEE句柄:              ~{handle_size} 字节")
        report.append(f"  Cache缓冲区:          {cache:,} 字节 (虚拟EEPROM)")
        report.append(f"  工作缓冲区:           {work_size} 字节 (2×颗粒)")
        report.append(f"  ─────────────────────────────")
        report.append(f"  总RAM占用:            {total_ram:,} 字节 ({total_ram/1024:.2f} KB)")
        report.append("")

        report.append("【性能评估】")
        if writes_before_gc > 0:
            report.append(f"  写入至GC触发:         ~{int(writes_before_gc)} 次写入操作")
            report.append(f"  预计年GC次数:         {gc_per_year:.1f} 次/年")
            report.append(f"  日均GC次数:           {gc_per_year/365:.2f} 次/天")

            if lifespan_years < 1000:
                report.append(f"  预期寿命:             {lifespan_years:.1f} 年")
                if lifespan_years < 5:
                    report.append(f"  ⚠️  警告: 寿命较短，建议：")
                    report.append(f"      • 增加pages_per_fee_page")
                    report.append(f"      • 减少写入频率")
                    report.append(f"      • 减小cache_size")
                elif lifespan_years < 10:
                    report.append(f"  ⚠️  提示: 寿命适中，可考虑优化")
                else:
                    report.append(f"  ✓ 寿命充足")
            else:
                report.append(f"  预期寿命:             > 1000 年 ✓")
        else:
            report.append(f"  ✗ 警告: 数据区太小，无法正常工作")
            report.append(f"    建议增加pages_per_fee_page或减小cache_size")

        report.append("")

        report.append("【容量分析】")
        max_data_capacity = max_records * record_data_size
        cache_usage = (cache / max_data_capacity) * 100 if max_data_capacity > 0 else 0
        report.append(f"  Cache占最大容量:      {cache_usage:.1f}%")

        if cache_usage > 90:
            report.append(f"  ✗ 严重: Cache过大，空间不足，必须增加Flash")
        elif cache_usage > 80:
            report.append(f"  ⚠️  警告: Cache占比过高，建议增加pages_per_fee_page")
        elif cache_usage < 20:
            report.append(f"  💡 提示: Cache占比较低，可减少Flash占用节省空间")
        else:
            report.append(f"  ✓ 配置合理")

        report.append("")

        report.append("【效率指标】")
        record_efficiency = (record_data_size / record_size) * 100
        report.append(f"  Record数据效率:       {record_efficiency:.1f}%")
        report.append(f"    ({record_data_size}B 有效数据 / {record_size}B 总大小)")

        header_overhead = (aligned_header / fee_page_size) * 100
        report.append(f"  Page Header开销:      {header_overhead:.2f}%")

        overall_efficiency = ((data_area - (max_records - cache//record_data_size) * record_size) / fee_page_size) * 100
        if overall_efficiency > 0:
            report.append(f"  总体利用率:           ~{overall_efficiency:.1f}%")

        report.append("")

        report.append("【优化建议】")
        suggestions = []

        if gran == FEE_MIN_GRANULARITY:
            suggestions.append("• 当前使用最小颗粒度，数据效率50%，可考虑增加到16字节提升效率")

        if lifespan_years < 5:
            suggestions.append("• 寿命不足5年，强烈建议增加Flash空间")

        if cache_usage > 80:
            suggestions.append("• Cache占比过高，增加pages_per_fee_page可提升性能")

        if cache_usage < 30 and pages_per > 1:
            suggestions.append("• Cache占比较低，可减少pages_per_fee_page节省Flash")

        if gran < 16 and record_data_size < 8:
            suggestions.append("• 考虑使用16字节颗粒度，可获得12字节数据（效率75%）")

        if suggestions:
            for s in suggestions:
                report.append(f"  {s}")
        else:
            report.append("  ✓ 当前配置无明显问题")

        report.append("")
        report.append("═" * 80)

        return "\n".join(report)

    def generate_c_code(self):
        """生成C配置代码"""
        try:
            flash_page = self.flash_page_size.get()
            gran = self.granularity.get()
            cache = self.cache_size.get()
            pages_per = self.pages_per_fee.get()
            max_erase = self.max_erase.get()

            # 验证颗粒度
            if gran < FEE_MIN_GRANULARITY:
                messagebox.showwarning("警告",
                    f"颗粒度({gran}B)小于最小要求({FEE_MIN_GRANULARITY}B)\n"
                    "生成的代码可能无法正常工作")

            code = []
            code.append("/* ═══════════════════════════════════════════════════════════════")
            code.append(" * FEE (Flash EEPROM Emulation) 配置代码")
            code.append(f" * MCU: {self.mcu_var.get()}")
            code.append(f" * 生成时间: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
            code.append(" * ═══════════════════════════════════════════════════════════════*/")
            code.append("")
            code.append("#include \"fee.h\"")
            code.append("")

            code.append("/* 设计验证 */")
            code.append(f"#if FEE_MIN_GRANULARITY > {gran}")
            code.append(f"#error \"FEE写入颗粒度必须 >= {FEE_MIN_GRANULARITY}字节\"")
            code.append("#endif")
            code.append("")

            code.append("/* ────────────────────────────────────────────────────────────── */")
            code.append("/* 1. 定义虚拟EEPROM数组 */")
            code.append("/* ────────────────────────────────────────────────────────────── */")
            code.append(f"static uint8_t virtual_eeprom[{cache}];")
            code.append("")

            code.append("/* ────────────────────────────────────────────────────────────── */")
            code.append("/* 2. 定义工作缓冲区 */")
            code.append("/* ────────────────────────────────────────────────────────────── */")
            code.append(f"static uint8_t work_buffer[FEE_WORK_SIZE({gran})];  /* {gran * 2} bytes */")
            code.append("")

            code.append("/* ────────────────────────────────────────────────────────────── */")
            code.append("/* 3. Flash操作函数（需要根据具体MCU实现） */")
            code.append("/* ────────────────────────────────────────────────────────────── */")
            code.append("")
            code.append("/* Flash擦除函数 */")
            code.append("static int flash_erase(uint32_t addr) {")
            code.append("    /* TODO: 实现Flash页擦除 */")
            code.append(f"    /* 提示: 擦除大小 = {flash_page} 字节 */")
            code.append("    return 0;  /* 0-成功, 非0-失败 */")
            code.append("}")
            code.append("")
            code.append("/* Flash写入函数 */")
            code.append("static int flash_write(uint32_t addr, const uint8_t *data, uint16_t len) {")
            code.append("    /* TODO: 实现Flash写入 */")
            code.append(f"    /* 提示: len必须是{gran}的整数倍 */")
            code.append("    return 0;  /* 0-成功, 非0-失败 */")
            code.append("}")
            code.append("")
            code.append("/* Flash读取函数 */")
            code.append("static int flash_read(uint32_t addr, uint8_t *data, uint16_t len) {")
            code.append("    /* TODO: 实现Flash读取 */")
            code.append("    memcpy(data, (void *)addr, len);")
            code.append("    return 0;  /* 0-成功, 非0-失败 */")
            code.append("}")
            code.append("")
            code.append("/* Flash操作接口 */")
            code.append("static const fee_flash_ops_t flash_ops = {")
            code.append("    .erase = flash_erase,")
            code.append("    .write = flash_write,")
            code.append("    .read  = flash_read")
            code.append("};")
            code.append("")

            code.append("/* ────────────────────────────────────────────────────────────── */")
            code.append("/* 4. FEE配置参数 */")
            code.append("/* ────────────────────────────────────────────────────────────── */")
            code.append("static const fee_config_t fee_config = {")
            code.append(f"    .flash_base         = (uint8_t *)0x08010000,  /* TODO: 修改为实际地址 */")
            code.append(f"    .pages_per_fee_page = {pages_per},            /* 每个FEE Page = {pages_per} × Flash Page */")
            code.append(f"    .flash_page_size    = {flash_page},          /* Flash Page大小 */")
            code.append(f"    .cache_size         = {cache},               /* 虚拟EEPROM大小 */")
            code.append(f"    .write_granularity  = {gran},                /* 写入颗粒度 (>= {FEE_MIN_GRANULARITY}) */")
            code.append(f"    .max_erase_count    = {max_erase},          /* 最大擦除次数 */")
            code.append(f"    .flash_ops          = &flash_ops")
            code.append("};")
            code.append("")

            code.append("/* ────────────────────────────────────────────────────────────── */")
            code.append("/* 5. FEE句柄（全局变量） */")
            code.append("/* ────────────────────────────────────────────────────────────── */")
            code.append("static fee_handle_t g_fee;")
            code.append("")

            code.append("/* ────────────────────────────────────────────────────────────── */")
            code.append("/* 6. 初始化函数 */")
            code.append("/* ────────────────────────────────────────────────────────────── */")
            code.append("int fee_system_init(void) {")
            code.append("    fee_status_t status;")
            code.append("    ")
            code.append("    /* 初始化FEE */")
            code.append("    status = fee_init(&g_fee, &fee_config, virtual_eeprom, work_buffer);")
            code.append("    ")
            code.append("    if (status != FEE_OK) {")
            code.append("        /* 初始化失败处理 */")
            code.append("        return -1;")
            code.append("    }")
            code.append("    ")
            code.append("    return 0;")
            code.append("}")
            code.append("")

            code.append("/* ────────────────────────────────────────────────────────────── */")
            code.append("/* 7. 使用示例 */")
            code.append("/* ────────────────────────────────────────────────────────────── */")
            code.append("")
            code.append("/* 示例1: 保存配置 */")
            code.append("void save_config(void) {")
            code.append("    typedef struct {")
            code.append("        uint32_t baudrate;")
            code.append("        uint16_t timeout;")
            code.append("        uint8_t  mode;")
            code.append("        uint8_t  reserved;")
            code.append("    } config_t;")
            code.append("    ")
            code.append("    config_t cfg = {")
            code.append("        .baudrate = 115200,")
            code.append("        .timeout  = 1000,")
            code.append("        .mode     = 1")
            code.append("    };")
            code.append("    ")
            code.append("    /* 写入到地址0 */")
            code.append("    fee_write(&g_fee, 0, (uint8_t *)&cfg, sizeof(cfg));")
            code.append("}")
            code.append("")
            code.append("/* 示例2: 读取配置 */")
            code.append("void load_config(void) {")
            code.append("    typedef struct {")
            code.append("        uint32_t baudrate;")
            code.append("        uint16_t timeout;")
            code.append("        uint8_t  mode;")
            code.append("        uint8_t  reserved;")
            code.append("    } config_t;")
            code.append("    ")
            code.append("    config_t cfg;")
            code.append("    ")
            code.append("    /* 方法1: 使用fee_read() */")
            code.append("    fee_read(&g_fee, 0, (uint8_t *)&cfg, sizeof(cfg));")
            code.append("    ")
            code.append("    /* 方法2: 直接访问虚拟EEPROM（更快） */")
            code.append("    memcpy(&cfg, &virtual_eeprom[0], sizeof(cfg));")
            code.append("    /* 或者: cfg = *(config_t *)&virtual_eeprom[0]; */")
            code.append("}")
            code.append("")
            code.append("/* 示例3: 增量计数器 */")
            code.append("void increment_counter(void) {")
            code.append("    uint32_t counter;")
            code.append("    ")
            code.append("    /* 读取当前值 */")
            code.append("    counter = *(uint32_t *)&virtual_eeprom[100];")
            code.append("    ")
            code.append("    /* 增加 */")
            code.append("    counter++;")
            code.append("    ")
            code.append("    /* 写回 */")
            code.append("    fee_write(&g_fee, 100, (uint8_t *)&counter, sizeof(counter));")
            code.append("}")
            code.append("")

            code.append("/* ────────────────────────────────────────────────────────────── */")
            code.append("/* 8. 状态查询 */")
            code.append("/* ────────────────────────────────────────────────────────────── */")
            code.append("void print_fee_status(void) {")
            code.append("    uint16_t erase_cnt, free_bytes, record_cnt;")
            code.append("    ")
            code.append("    if (fee_get_info(&g_fee, &erase_cnt, &free_bytes, &record_cnt) == FEE_OK) {")
            code.append("        printf(\"FEE状态:\\n\");")
            code.append("        printf(\"  擦除次数: %u\\n\", erase_cnt);")
            code.append("        printf(\"  剩余空间: %u 字节\\n\", free_bytes);")
            code.append("        printf(\"  记录数量: %u\\n\", record_cnt);")
            code.append("    }")
            code.append("}")
            code.append("")

            code.append("/* ═══════════════════════════════════════════════════════════════")
            code.append(" * 配置摘要:")
            code.append(f" * - Flash占用: {flash_page * pages_per * 2:,} 字节 ({flash_page * pages_per * 2 / 1024:.1f} KB)")
            code.append(f" * - RAM占用:   {28 + cache + gran * 2:,} 字节 ({(28 + cache + gran * 2) / 1024:.2f} KB)")
            code.append(f" * - Record数据: {gran - FEE_RECORD_HEADER_SIZE} 字节/条 (效率: {(gran-FEE_RECORD_HEADER_SIZE)/gran*100:.1f}%)")
            code.append(" * ═══════════════════════════════════════════════════════════════*/")

            self.code_text.config(state='normal')
            self.code_text.delete(1.0, tk.END)
            self.code_text.insert(1.0, "\n".join(code))
            self.code_text.config(state='disabled')

        except Exception as e:
            messagebox.showerror("代码生成错误", str(e))

    def copy_to_clipboard(self):
        """复制到剪贴板"""
        try:
            code = self.code_text.get(1.0, tk.END)
            self.root.clipboard_clear()
            self.root.clipboard_append(code)
            messagebox.showinfo("成功", "代码已复制到剪贴板")
        except Exception as e:
            messagebox.showerror("复制失败", str(e))

    def save_code(self):
        """保存代码到文件"""
        filename = filedialog.asksaveasfilename(
            defaultextension=".c",
            filetypes=[("C文件", "*.c"), ("头文件", "*.h"), ("所有文件", "*.*")]
        )
        if filename:
            try:
                code = self.code_text.get(1.0, tk.END)
                with open(filename, 'w', encoding='utf-8') as f:
                    f.write(code)
                messagebox.showinfo("成功", f"代码已保存到:\n{filename}")
            except Exception as e:
                messagebox.showerror("保存失败", str(e))

def main():
    root = tk.Tk()
    app = FEECalculator(root)
    root.mainloop()

if __name__ == "__main__":
    main()

