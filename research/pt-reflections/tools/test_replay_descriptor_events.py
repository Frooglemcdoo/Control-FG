import io
import unittest
import zipfile
from replay_descriptor_events import submitted_lines, functions, descriptor


def srv(resource):
    r = f'GetResource({resource}).Get()' if resource is not None else 'nullptr'
    return f'CreateShaderResourceView_Buffer({r}, GetCpuDescriptor(g_descriptorHeap_7.Get(), 5), DXGI_FORMAT_UNKNOWN, D3D12_SRV_DIMENSION_BUFFER, 5768, 0, 3, 336, D3D12_BUFFER_SRV_FLAG_NONE);'


def function(name, body):
    return f'void {name}()\n{{\n    '+body.replace('\n', '\n    ')+'\n    }\n'


class ReplayTests(unittest.TestCase):
    def replay(self, edits):
        data = io.BytesIO()
        files = {
            'Descriptors_000.cpp': function('CreateDescriptors_7_0', srv(100)),
            'ModifyDescriptors_000.cpp': function('ModifyDescriptors_7_0', srv(200))+function('ModifyDescriptors_7_1', srv(None)),
            'CommandLists_000.cpp': function('PopulateCommandList_9_1_0', 'GetCommandList(9)->Reset(nullptr, nullptr);\nGetCommandList(9)->Dispatch(1, 1, 1);'),
            'RenderFrame.cpp': 'void RenderFrame()\n{\n    RenderFrame_000();\n}\n',
            'RenderFrameWorker_000.cpp': function('RenderFrame_000', edits),
        }
        with zipfile.ZipFile(data, 'w') as z:
            for name, text in files.items():
                z.writestr('project/'+name, text)
        data.seek(0)
        with zipfile.ZipFile(data) as z:
            return [(line, dict(descriptors[('7', 5)]), submit) for _, _, line, descriptors, submit in submitted_lines(z, 'project/') if 'Dispatch(' in line]

    def test_after_recording_before_submission_and_null_overwrite(self):
        submit = 'ID3D12CommandList* commandLists[1];\ncommandLists[0] = GetCommandList(9).Get();\nGetCommandQueue(1)->ExecuteCommandLists(_countof(commandLists), commandLists);'
        events = self.replay('PopulateCommandList_9_1_0();\nModifyDescriptors_7_0();\n'+submit+'\nModifyDescriptors_7_1();\n'+submit)
        self.assertEqual([e[1]['resource'] for e in events], ['200', None])
        self.assertEqual([e[2]['submission'] for e in events], [1, 2])

    def test_missing_update_rejected(self):
        with self.assertRaisesRegex(ValueError, 'missing descriptor update'):
            self.replay('ModifyDescriptors_7_99();')

    def test_non_srv_overwrite_is_not_a_stale_srv(self):
        key, value = descriptor('CreateConstantBufferView(GetCpuDescriptor(g_descriptorHeap_7.Get(), 5), GetGpuva(12, 256), 256);')
        self.assertEqual(key, ('7', 5))
        self.assertEqual((value['kind'], value['resource'], value['offset']), ('cbv', '12', 256))

    def test_nested_initializer_and_indented_final_brace(self):
        f = functions({'file.cpp': function('Example', 'int v[]={1,2};\n{\n call();\n}')})
        self.assertEqual(len(f['Example']), 4)


if __name__ == '__main__':
    unittest.main()
