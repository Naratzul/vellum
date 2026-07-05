import { spawn } from 'child_process';
import * as fs from 'fs';
import * as path from 'path';
import {
	OutputChannel,
	TextDocument,
	WorkspaceConfiguration,
	WorkspaceFolder,
	window,
	workspace,
} from 'vscode';

function getCompilerExecutableName(): string {
	return process.platform === 'win32' ? 'vellum.exe' : 'vellum';
}

export function resolveCompilerPath(cfg: WorkspaceConfiguration): string {
	const override = cfg.get<string>('compilerPath', '').trim();
	if (override) {
		return path.normalize(override);
	}
	return getCompilerExecutableName();
}

export function buildCompileArgs(
	filePath: string,
	importPaths: string[],
	outputDirectory: string | undefined,
): string[] {
	const args = ['-f', filePath];

	if (importPaths.length > 0) {
		args.push('-i', importPaths.join(';'));
	}

	if (outputDirectory) {
		args.push('-o', outputDirectory);
	}

	return args;
}

function getConfigForDocument(
	document: TextDocument,
): { cfg: WorkspaceConfiguration; wsFolder: WorkspaceFolder | undefined } {
	const wsFolder = workspace.getWorkspaceFolder(document.uri);
	const cfg = workspace.getConfiguration('vellum', wsFolder?.uri);
	return { cfg, wsFolder };
}

async function saveIfDirty(document: TextDocument): Promise<boolean> {
	if (!document.isDirty) {
		return true;
	}
	return document.save();
}

function runCompiler(
	compilerPath: string,
	args: string[],
	cwd: string | undefined,
	output: OutputChannel,
): Promise<number> {
	return new Promise((resolve, reject) => {
		output.appendLine(`> ${compilerPath} ${args.join(' ')}`);
		output.show(true);

		const child = spawn(compilerPath, args, {
			cwd,
			shell: process.platform === 'win32',
		});

		child.stdout.on('data', (data: Buffer | string) => {
			output.append(data.toString());
		});

		child.stderr.on('data', (data: Buffer | string) => {
			output.append(data.toString());
		});

		child.on('error', (err) => {
			reject(err);
		});

		child.on('close', (code) => {
			resolve(code ?? 1);
		});
	});
}

export async function compileDocument(
	document: TextDocument,
	expandWorkspaceFolder: (input: string, wsFolder: WorkspaceFolder | undefined) => string,
	output: OutputChannel,
): Promise<void> {
	if (document.languageId !== 'vellum' || document.uri.scheme !== 'file') {
		void window.showErrorMessage('Vellum: open a saved .vel file to compile.');
		return;
	}

	if (!(await saveIfDirty(document))) {
		void window.showErrorMessage('Vellum: could not save the file before compiling.');
		return;
	}

	const filePath = document.uri.fsPath;
	const { cfg, wsFolder } = getConfigForDocument(document);

	const importPaths = cfg
		.get<string[]>('importPaths', [])
		.map((p) => expandWorkspaceFolder(p, wsFolder))
		.map((p) => path.normalize(p));

	const outputDirectoryRaw = cfg.get<string>('outputDirectory', '').trim();
	const outputDirectory = outputDirectoryRaw
		? path.normalize(expandWorkspaceFolder(outputDirectoryRaw, wsFolder))
		: undefined;

	const compilerPath = resolveCompilerPath(cfg);
	if (compilerPath !== getCompilerExecutableName() && !fs.existsSync(compilerPath)) {
		void window.showErrorMessage(`Vellum: compiler not found at: ${compilerPath}`);
		return;
	}

	const args = buildCompileArgs(filePath, importPaths, outputDirectory);

	try {
		const exitCode = await runCompiler(
			compilerPath,
			args,
			wsFolder?.uri.fsPath,
			output,
		);

		if (exitCode === 0) {
			void window.showInformationMessage(`Vellum: compiled ${path.basename(filePath)}`);
		} else {
			void window.showErrorMessage(
				`Vellum: compilation failed (exit code ${exitCode}). See the Vellum Compiler output for details.`,
			);
		}
	} catch (err) {
		const message = err instanceof Error ? err.message : String(err);
		output.appendLine(`Failed to run compiler: ${message}`);
		output.show(true);
		void window.showErrorMessage(`Vellum: failed to run compiler — ${message}`);
	}
}

export async function compileActiveFile(
	expandWorkspaceFolder: (input: string, wsFolder: WorkspaceFolder | undefined) => string,
	output: OutputChannel,
): Promise<void> {
	const editor = window.activeTextEditor;
	if (!editor) {
		void window.showErrorMessage('Vellum: no active editor.');
		return;
	}

	await compileDocument(editor.document, expandWorkspaceFolder, output);
}
