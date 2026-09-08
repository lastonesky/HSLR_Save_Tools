use ratatui::Frame;
use ratatui::layout::{Constraint, Direction, Layout, Rect};
use ratatui::style::{Color, Modifier, Style};
use ratatui::widgets::{Block, Borders, Cell, Row, Table, Paragraph};

use crate::editor::EditorApp;
use crate::save::*;

/// 绘制全角色一览页
pub fn draw(f: &mut Frame, app: &EditorApp, area: Rect) {
    let chunks = Layout::default()
        .direction(Direction::Vertical)
        .constraints([
            Constraint::Min(10),  // 角色表格
            Constraint::Length(3), // 说明
        ])
        .split(area);

    // 角色表格
    draw_roster_table(f, app, chunks[0]);

    // 说明
    draw_note(f, app, chunks[1]);
}

/// 绘制角色表格
fn draw_roster_table(f: &mut Frame, app: &EditorApp, area: Rect) {
    let header_cells = ["Key", "ID", "名称", "等级", "HP", "MaxHP", "物攻", "魔攻", "防御", "阵营"]
        .iter()
        .map(|h| Cell::from(*h).style(Style::default().fg(Color::Yellow).add_modifier(Modifier::BOLD)));
    let header = Row::new(header_cells).height(1);

    let mut rows = Vec::new();

    // 使用所有战场实体
    for (key, entity) in &app.all_stage_entities {
        let pid = get_i64(entity, "PlayerId").map_or("?".to_string(), |v| v.to_string());
        let name = get_string(entity, "Name").unwrap_or_else(|| key.clone());
        let level = get_i64(entity, "Level").map_or("?".to_string(), |v| v.to_string());
        let hp = get_i64(entity, "Hp").map_or("?".to_string(), |v| v.to_string());
        let max_hp = get_i64(entity, "MaxHp").map_or("?".to_string(), |v| v.to_string());

        let fight_attr = get_map(entity, "FightAttr");
        let phys_atk = fight_attr
            .and_then(|fa| get_i64(fa, "PhysicalAttack"))
            .map_or("?".to_string(), |v| v.to_string());
        let mag_atk = fight_attr
            .and_then(|fa| get_i64(fa, "MagicAttack"))
            .map_or("?".to_string(), |v| v.to_string());
        let defense = fight_attr
            .and_then(|fa| get_i64(fa, "Defense"))
            .map_or("?".to_string(), |v| v.to_string());

        let camp = get_i64(entity, "Camp").map_or("?".to_string(), |v| match v {
            1 => "敌方".to_string(),
            2 => "我方".to_string(),
            3 => "中立".to_string(),
            _ => v.to_string(),
        });

        // 检查当前角色的字段是否被选中
        let is_selected_level = app.edit_fields.get(app.selected_field)
            .map(|f| f.path == vec!["stage_entity".to_string(), key.clone(), "Level".to_string()])
            .unwrap_or(false);
        let is_selected_hp = app.edit_fields.get(app.selected_field)
            .map(|f| f.path == vec!["stage_entity".to_string(), key.clone(), "Hp".to_string()])
            .unwrap_or(false);
        let is_selected_max_hp = app.edit_fields.get(app.selected_field)
            .map(|f| f.path == vec!["stage_entity".to_string(), key.clone(), "MaxHp".to_string()])
            .unwrap_or(false);

        // 根据选中状态设置样式
        let level_style = if app.editing && is_selected_level {
            Style::default().fg(Color::Black).bg(Color::Yellow)
        } else if is_selected_level {
            Style::default().fg(Color::Black).bg(Color::Cyan)
        } else {
            Style::default().fg(Color::White)
        };

        let hp_style = if app.editing && is_selected_hp {
            Style::default().fg(Color::Black).bg(Color::Yellow)
        } else if is_selected_hp {
            Style::default().fg(Color::Black).bg(Color::Cyan)
        } else {
            Style::default().fg(Color::White)
        };

        let max_hp_style = if app.editing && is_selected_max_hp {
            Style::default().fg(Color::Black).bg(Color::Yellow)
        } else if is_selected_max_hp {
            Style::default().fg(Color::Black).bg(Color::Cyan)
        } else {
            Style::default().fg(Color::White)
        };

        // 获取显示值
        let level_display = if app.editing && is_selected_level {
            app.input_buffer.clone()
        } else {
            level
        };
        let hp_display = if app.editing && is_selected_hp {
            app.input_buffer.clone()
        } else {
            hp
        };
        let max_hp_display = if app.editing && is_selected_max_hp {
            app.input_buffer.clone()
        } else {
            max_hp
        };

        let row = Row::new(vec![
            Cell::from(key.clone()),
            Cell::from(pid),
            Cell::from(name),
            Cell::from(level_display).style(level_style),
            Cell::from(hp_display).style(hp_style),
            Cell::from(max_hp_display).style(max_hp_style),
            Cell::from(phys_atk),
            Cell::from(mag_atk),
            Cell::from(defense),
            Cell::from(camp),
        ]);

        rows.push(row);
    }

    let table = Table::new(
        rows,
        [
            Constraint::Length(12), // Key
            Constraint::Length(6),  // ID
            Constraint::Length(14), // 名称
            Constraint::Length(6),  // 等级
            Constraint::Length(8),  // HP
            Constraint::Length(8),  // MaxHP
            Constraint::Length(8),  // 物攻
            Constraint::Length(8),  // 魔攻
            Constraint::Length(8),  // 防御
            Constraint::Length(6),  // 阵营
        ],
    )
    .header(header)
    .block(Block::default().borders(Borders::ALL).title(format!("全角色一览 (共{}个)", app.all_stage_entities.len())))
    .highlight_style(Style::default().bg(Color::DarkGray).add_modifier(Modifier::BOLD))
    .highlight_symbol(">> ");

    f.render_widget(table, area);
}

/// 绘制说明
fn draw_note(f: &mut Frame, app: &EditorApp, area: Rect) {
    let note_text = if app.editing {
        "编辑模式: 输入数值后 Enter 确认 | Esc 取消".to_string()
    } else if !app.edit_fields.is_empty() {
        "↑↓: 选择字段 | Enter: 编辑 | +/-: 快捷增减".to_string()
    } else {
        "💡 全角色一览支持编辑等级、HP、MaxHP".to_string()
    };

    let note = Paragraph::new(note_text)
        .style(Style::default().fg(Color::DarkGray))
        .block(Block::default().borders(Borders::ALL).title("说明"));

    f.render_widget(note, area);
}
