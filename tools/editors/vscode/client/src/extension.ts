import * as fs from 'fs';
import * as path from 'path';
import { commands, window, workspace, WorkspaceFolder, ExtensionContext, WorkspaceConfiguration } from 'vscode';
import * as process from 'process'

import {
	LanguageClient,
	LanguageClientOptions,
	ServerOptions,
	TransportKind
} from 'vscode-languageclient/node';

import { compileActiveFile } from './compile';

let client: LanguageClient;

function expandWorkspaceFolder(input: string, wsFolder: WorkspaceFolder | undefined): string {
	if (!wsFolder) return input;
  	return input.replace(/\$\{workspaceFolder\}/g, wsFolder.uri.fsPath);
}

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

export function activate(context: ExtensionContext) {
	const wsFolder =
		(window.activeTextEditor ? workspace.getWorkspaceFolder(window.activeTextEditor.document.uri) : undefined)
		?? workspace.workspaceFolders?.[0];

	const cfg = workspace.getConfiguration("vellum")
	const importPaths = cfg.get<string[]>("importPaths", [])
		.map(p => expandWorkspaceFolder(p, wsFolder))
		.map(p => path.normalize(p))

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
		synchronize: {
			configurationSection: 'vellum',
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
		commands.registerCommand('vellum.compile', () =>
			compileActiveFile(expandWorkspaceFolder, compileOutput)),
		compileOutput,
	);
}

export function deactivate(): Thenable<void> | undefined {
	if (!client) {
		return undefined;
	}
	return client.stop();
}
