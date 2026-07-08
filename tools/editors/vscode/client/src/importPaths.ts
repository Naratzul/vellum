import * as path from 'path';
import * as process from 'process';
import { workspace, window, WorkspaceConfiguration, WorkspaceFolder } from 'vscode';

export function expandWorkspaceFolder(
	input: string,
	wsFolder: WorkspaceFolder | undefined,
): string {
	if (!wsFolder) {
		return input;
	}
	return input.replace(/\$\{workspaceFolder\}/g, wsFolder.uri.fsPath);
}

function pathKey(p: string): string {
	const normalized = path.normalize(p);
	return process.platform === 'win32' ? normalized.toLowerCase() : normalized;
}

export function dedupePaths(paths: string[]): string[] {
	const seen = new Set<string>();
	const out: string[] = [];

	for (const p of paths) {
		const key = pathKey(p);
		if (seen.has(key)) {
			continue;
		}
		seen.add(key);
		out.push(path.normalize(p));
	}

	return out;
}

function getImplicitWorkspaceImportPaths(): string[] {
	if (workspace.workspaceFolders && workspace.workspaceFolders.length > 0) {
		return workspace.workspaceFolders.map((folder) => path.normalize(folder.uri.fsPath));
	}
	return [];
}

export function getPrimaryWorkspaceFolder(): WorkspaceFolder | undefined {
	return (
		(window.activeTextEditor
			? workspace.getWorkspaceFolder(window.activeTextEditor.document.uri)
			: undefined) ?? workspace.workspaceFolders?.[0]
	);
}

export function resolveImportPaths(
	cfg: WorkspaceConfiguration,
	wsFolder?: WorkspaceFolder,
): string[] {
	const implicit = getImplicitWorkspaceImportPaths();
	const configured = cfg
		.get<string[]>('importPaths', [])
		.map((p) => expandWorkspaceFolder(p, wsFolder))
		.map((p) => path.normalize(p));

	return dedupePaths([...implicit, ...configured]);
}
