import * as fs from 'fs';
import * as path from 'path';
import { commands, window, workspace, ExtensionContext, WorkspaceConfiguration } from 'vscode';
import * as process from 'process';

import {
	LanguageClient,
	LanguageClientOptions,
	ServerOptions,
	TransportKind
} from 'vscode-languageclient/node';

import { compileActiveFile } from './compile';
import { getPrimaryWorkspaceFolder, resolveImportPaths } from './importPaths';

let client: LanguageClient;

function getServerPlatformDir(): string {
	if (process.platform === 'win32') {
		return 'win32-x64';
	} else if (process.platform === 'darwin') {
		return process.arch === 'arm64' ? 'darwin-arm64' : 'darwin-x64';
	} else if (process.platform === 'linux') {
		return process.arch === 'arm64' ? 'linux-arm64' : 'linux-x64';
	}
	throw new Error(`Unsupported platform: ${process.platform}`);
}

function getServerExecutableName(): string {
	if (process.platform == "win32") {
		return "vellum-lsp.exe";
	} else if (process.platform == "darwin" || process.platform == "linux") {
		return "vellum-lsp";
	}
	throw new Error(`Unsupported platform: ${process.platform}`);
}

function resolveServerModule(context: ExtensionContext, cfg: WorkspaceConfiguration): string | undefined {
	const override = cfg.get<string>('languageServerPath', '').trim();
	if (override) {
		return path.normalize(override);
	}

	let platformDir: string;
	try {
		platformDir = getServerPlatformDir();
	} catch (err) {
		const message = err instanceof Error ? err.message : String(err);
		void window.showErrorMessage(`Vellum: ${message}`);
		return undefined;
	}

	const serverDir = path.join(context.extensionPath, 'server', platformDir);
	return path.join(serverDir, getServerExecutableName());
}

function syncImportPathsToServer(): void {
	if (!client) {
		return;
	}

	const wsFolder = getPrimaryWorkspaceFolder();
	const cfg = workspace.getConfiguration('vellum', wsFolder?.uri);
	const importPaths = resolveImportPaths(cfg, wsFolder);

	void client.sendNotification('workspace/didChangeConfiguration', {
		settings: { vellum: { importPaths } },
	});
}

export function activate(context: ExtensionContext) {
	const wsFolder = getPrimaryWorkspaceFolder();

	const cfg = workspace.getConfiguration("vellum");
	const importPaths = resolveImportPaths(cfg, wsFolder);

	const serverModule = resolveServerModule(context, cfg);
	if (!serverModule) {
		return;
	}

	if (!fs.existsSync(serverModule)) {
		void window.showErrorMessage(
			`Vellum language server not found at: ${serverModule}`,
		);
		return;
	}

	const serverOptions: ServerOptions = {
		command: serverModule,
		transport: TransportKind.stdio,
	};

	const lspOutput = window.createOutputChannel('Vellum Language Server');
	lspOutput.appendLine('Vellum Language Server logs:');

	const clientOptions: LanguageClientOptions = {
		documentSelector: [{ scheme: 'file', language: 'vellum' }],
		initializationOptions: {
			importPaths: importPaths
		},
		workspaceFolder: wsFolder,
		outputChannel: lspOutput,
	};

	client = new LanguageClient(
		'vellum',
		'vellum',
		serverOptions,
		clientOptions
	);

	void client.start().catch((err: unknown) => {
		const message = err instanceof Error ? err.message : String(err);
		lspOutput.appendLine(`Failed to start language server: ${message}`);
		lspOutput.show();
		void window.showErrorMessage(`Vellum: failed to start language server — ${message}`);
	});

	const compileOutput = window.createOutputChannel('Vellum Compiler');
	context.subscriptions.push(
		commands.registerCommand('vellum.compile', () => compileActiveFile(compileOutput)),
		compileOutput,
		workspace.onDidChangeConfiguration((e) => {
			if (e.affectsConfiguration('vellum')) {
				syncImportPathsToServer();
			}
		}),
		workspace.onDidChangeWorkspaceFolders(() => {
			syncImportPathsToServer();
		}),
	);
}

export function deactivate(): Thenable<void> | undefined {
	if (!client) {
		return undefined;
	}
	return client.stop();
}
